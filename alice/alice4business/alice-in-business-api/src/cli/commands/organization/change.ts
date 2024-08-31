import commander from 'commander';
import {
    _makeChoiceQuestion,
    getConnectOrganizationsChoiceList,
    getOrganizationsChoiceList,
    makeAChoice,
    withErrors,
} from '../../utils';
import inquirer from 'inquirer';
import { OrganizationSchema } from '../../../db/tables/organization';
import { sequelize, Organization, Permission } from '../../../db';
import Connect from '../../../services/connect';
import ip from 'ip';
import chalk from 'chalk';
import { Source } from '../../../db/tables/permission';

export default (program: commander.Command) => {
    program
        .command('organizations:edit')
        .description('Change organization')
        .action(
            withErrors(async () => {
                const organizationId = await makeAChoice(
                    await getOrganizationsChoiceList(),
                    'Device owner organization',
                    'autocomplete',
                );

                const organization = await Organization.findByPk(organizationId);
                if (!organization) {
                    return;
                }

                const newData = await inquirer.prompt<Partial<OrganizationSchema>>([
                    {
                        type: 'input',
                        name: 'name',
                        default: organization.name,
                        message: 'Organization name',
                        validate: (val) => (val ? true : 'value is required'),
                        filter: (input) => input.trim(),
                    },
                    {
                        ..._makeChoiceQuestion(
                            [
                                {
                                    name: `--- keep current value ${
                                        organization.connectOrgId || `NOT BOUND`
                                    } ---`,
                                    value: organization.connectOrgId,
                                },
                                { name: '---  do not bind to Connect ---', value: null },
                                ...(await getConnectOrganizationsChoiceList()),
                            ],
                            'autocomplete',
                        ),
                        name: 'connectOrgId',
                        message: 'Connect organization id',
                    },
                    {
                        type: 'input',
                        name: 'templateUrl',
                        default: organization.templateUrl,
                        message:
                            'Kolonkish agreement template URL (https://s3.mds.yandex.net/dialogs/b2b/agreement-*)',
                        filter: (input) => input.trim() || undefined,
                    },
                    {
                        type: 'input',
                        name: 'imageUrl',
                        default: organization.imageUrl || '',
                        message:
                            'Organization preview image (for TV)',
                        filter: (input) => input.trim() || undefined,
                    },
                    {
                        type: 'input',
                        name: 'infoUrl',
                        default: organization.infoUrl || '',
                        message:
                            'Organization info web page (for TV)',
                        filter: (input) => input.trim() || undefined,
                    },
                    {
                        type: 'input',
                        name: 'infoTitle',
                        default: organization.infoTitle || '',
                        message:
                            'Organization info thumbnail title',
                        filter: (input) => input.trim() || undefined,
                    },
                    {
                        type: 'input',
                        name: 'infoSubtitle',
                        default: organization.infoSubtitle || '',
                        message:
                            'Organization info thumbnail subtitle',
                        filter: (input) => input.trim() || undefined,
                    },
                    {
                        ..._makeChoiceQuestion(
                            [
                                { name: organization.usesRooms ? 'yes (current)': 'yes', value: true },
                                { name: !organization.usesRooms ? 'no (current)': 'no', value: false },
                            ],
                        ),
                        name: 'usesRooms',
                        message: 'Use room feature? (experimental)',
                    },
                    {
                        type: 'input',
                        name: 'maxStationVolume',
                        default: organization.maxStationVolume ||'',
                        validate: (val) => {
                            if (val === null) {
                                return true
                            }
                            if (isNaN(val)) {
                                return "valid number is required"
                            }
                            return (val > 0 && val <= 10) ? true : "required value in range [1 - 10]"
                        },
                        filter: (input) => {
                            if (input === '') {
                                return null
                            }
                            return parseInt(input, 10)
                        }
                    }
                ]);
                const oldConnectId = organization.connectOrgId;
                await sequelize.transaction(async (transaction) => {
                    await organization.update(newData, { transaction });

                    if (organization.connectOrgId !== oldConnectId) {
                        await Permission.destroy({
                            where: {
                                organizationId: organization.id,
                                source: Source.Connect,
                            },
                            transaction,
                        });
                    }
                });

                if (organization.connectOrgId !== oldConnectId) {
                    if (oldConnectId) {
                        try {
                            const {
                                admin_id,
                            } = await Connect.getOrganization(
                                oldConnectId,
                                ['admin_id'],
                                { tvmSrc: 'connect-controller' },
                            );

                            await Connect.deleteResource(oldConnectId, organization.id, {
                                uid: admin_id!,
                                ip: ip.address(),
                            });
                        } catch (e) {
                            console.warn(
                                chalk.red(
                                    `Failed to delete resource from Connect organization ${organization.connectOrgId}. Sync it later.\n`,
                                ),
                            );
                        }
                    }

                    if (organization.connectOrgId) {
                        try {
                            await Connect.createResource(organization.connectOrgId, {
                                id: organization.id,
                            });
                        } catch (e) {
                            console.warn(
                                chalk.red(
                                    `Failed to create resource in Connect organization ${newData.connectOrgId}. Sync it later.\n`,
                                ),
                            );
                        }
                    }
                }
            }),
        );
};

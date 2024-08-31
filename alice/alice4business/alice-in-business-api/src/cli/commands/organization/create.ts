import commander from 'commander';
import { Organization, Permission } from '../../../db';
import {
    PermissionSchema,
    Source,
    Types as PermissionTypes,
} from '../../../db/tables/permission';
import {
    _makeChoiceQuestion,
    getConnectOrganizationsChoiceList,
    getUserChoiceList,
    makeAChoice,
    makeAConfirm,
    withErrors,
} from '../../utils';
import inquirer from 'inquirer';
import { OrganizationSchema } from '../../../db/tables/organization';
import Connect from '../../../services/connect';
import { GotError } from 'got';
import config from '../../../lib/config';
import chalk from 'chalk';

export default (program: commander.Command) => {
    program
        .command('organizations:create')
        .description('Create organization')
        .action(
            withErrors(async () => {
                const organizationData = await inquirer.prompt<
                    Partial<OrganizationSchema>
                >([
                    {
                        type: 'input',
                        name: 'name',
                        message: 'Organization name',
                        validate: (val) => (val ? true : 'value is required'),
                        filter: (input) => input.trim(),
                    },
                    {
                        ..._makeChoiceQuestion(
                            [
                                { name: '--- do not bind to Connect ---', value: null },
                                ...(await getConnectOrganizationsChoiceList()),
                            ],
                            'autocomplete',
                        ),
                        name: 'connectOrgId',
                        message: 'Connect organization id',
                    },
                    {
                        type: 'input',
                        name: 'templateURL',
                        message:
                            'Kolonkish agreement template URL (https://s3.mds.yandex.net/dialogs/b2b/agreement-*)',
                        filter: (input) => input.trim() || undefined,
                    },
                    {
                        type: 'input',
                        name: 'imageUrl',
                        default: '',
                        message:
                            'Organization preview image (for TV)',
                        filter: (input) => input.trim() || undefined,
                    },
                    {
                        type: 'input',
                        name: 'infoUrl',
                        default: '',
                        message:
                            'Organization info web page (for TV)',
                        filter: (input) => input.trim() || undefined,
                    },
                    {
                        type: 'input',
                        name: 'infoTitle',
                        default: 'Добро пожаловать в отель!',
                        message:
                            'Organization info thumbnail title',
                        filter: (input) => input.trim() || undefined,
                    },
                    {
                        type: 'input',
                        name: 'infoSubtitle',
                        default: 'Нажмите и посмотрите, что интересного в отеле и рядом',
                        message:
                            'Organization info thumbnail subtitle',
                        filter: (input) => input.trim() || undefined,
                    },
                    {
                        ..._makeChoiceQuestion(
                            [
                                { name: 'yes', value: true },
                                { name: 'no', value: false },
                            ],
                        ),
                        name: 'usesRooms',
                        message: 'Use room feature? (experimental)',
                    }
                ]);

                const organization = await Organization.create(organizationData);
                console.log(`organization id: ${chalk.whiteBright(organization.id)}`);

                if (organization.connectOrgId) {
                    await Connect.createResource(organization.connectOrgId, {
                        id: organization.id,
                    }).catch((err: GotError) => {
                        if (err.name === 'HTTPError') {
                            if (err.statusCode === 409) {
                                return;
                            }
                            if (err.statusCode === 403) {
                                console.warn(
                                    `${config.connect.selfSlug} service is disabled for Connect organization ${organization.connectOrgId}. Please sync resources later`,
                                );
                                return;
                            }
                        }
                        throw err;
                    });
                }

                if (await makeAConfirm('Do you want bind users to new organization?')) {
                    const userIds = await makeAChoice(
                        await getUserChoiceList(),
                        'Choice user',
                        'checkbox',
                    );

                    const permissions = [] as PermissionSchema[];
                    for (const userId of userIds) {
                        for (const type of Object.keys(
                            PermissionTypes,
                        ) as (keyof typeof PermissionTypes)[]) {
                            permissions.push({
                                organizationId: organization.id,
                                uid: parseInt(userId, 10),
                                type,
                                source: Source.Native,
                            });
                        }
                    }
                    await Permission.bulkCreate(permissions as PermissionSchema[], {
                        ignoreDuplicates: true,
                    });
                }
            }),
        );
};

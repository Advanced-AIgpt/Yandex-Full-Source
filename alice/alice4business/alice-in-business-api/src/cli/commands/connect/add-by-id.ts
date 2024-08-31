import commander from 'commander';
import { makeAConfirm, withErrors } from '../../utils';
import Connect from '../../../services/connect';
import { ConnectOrganization } from '../../../db';
import { ConnectOrganizationSchema } from '../../../db/tables/connectOrganization';
import inquirer = require('inquirer');

export default (program: commander.Command) => {
    program
        .command('connect:add-by-id')
        .description('Add Connect organization by id')
        .action(
            withErrors(async () => {
                const { orgId } = await inquirer.prompt([
                    {
                        type: 'number',
                        message: 'Connect organization id',
                        name: 'orgId',
                        filter(input) {
                            return isNaN((input as any) as number) ? undefined : input;
                        },
                        validate(input) {
                            return isNaN((input as any) as number)
                                ? 'org_id must be a number'
                                : true;
                        },
                    },
                ]);

                const { name } = await Connect.getOrganization(orgId, ['name'], {
                    tvmSrc: 'connect-controller',
                });

                if (
                    await makeAConfirm(`Add Connect organization "${name}" (${orgId})?`)
                ) {
                    await ConnectOrganization.bulkCreate(
                        [{ id: orgId, name }] as ConnectOrganizationSchema[],
                        {
                            ignoreDuplicates: true,
                        },
                    );
                }
            }),
        );
};

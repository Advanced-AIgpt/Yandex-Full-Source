import commander from 'commander';
import { Permission } from '../../../db';
import {
    PermissionSchema,
    Source,
    Types as PermissionTypes,
} from '../../../db/tables/permission';
import {
    getOrganizationsChoiceList,
    getUserChoiceList,
    makeAChoice,
    withErrors,
} from '../../utils';

export default (program: commander.Command) => {
    program
        .command('users:bind')
        .description('Bind user to organization')
        .action(
            withErrors(async () => {
                const userIdList = await makeAChoice(
                    await getUserChoiceList(),
                    'Choice user',
                    'checkbox',
                );

                const organizationIdList = await makeAChoice(
                    await getOrganizationsChoiceList(true),
                    'Choice organization',
                    'checkbox',
                );

                const permissions = [] as PermissionSchema[];
                for (const organizationId of organizationIdList) {
                    for (const userId of userIdList) {
                        for (const type of Object.keys(
                            PermissionTypes,
                        ) as (keyof typeof PermissionTypes)[]) {
                            permissions.push({
                                organizationId,
                                uid: parseInt(userId, 10),
                                type,
                                source: Source.Native,
                            });
                        }
                    }
                }
                await Permission.bulkCreate(permissions as PermissionSchema[], {
                    ignoreDuplicates: true,
                });
            }),
        );
};

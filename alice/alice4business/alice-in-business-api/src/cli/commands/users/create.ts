import commander from 'commander';
import {
    askForInput,
    fetchUid,
    getOrganizationsChoiceList,
    makeAChoice,
    makeAConfirm,
    withErrors,
} from '../../utils';
import { Permission, User } from '../../../db';
import {
    PermissionSchema,
    Source,
    Types as PermissionTypes,
} from '../../../db/tables/permission';
import { UserSchema } from '../../../db/tables/user';

export default (program: commander.Command) => {
    program
        .command('users:create')
        .description('Create user')
        .action(
            withErrors(async () => {
                const login = await askForInput('Input user login');
                const uid = await fetchUid(login);
                console.log(`User id: ${uid}`);

                if (await makeAConfirm('Bind user to organizations?')) {
                    await User.bulkCreate([{ id: uid, login }] as UserSchema[], {
                        ignoreDuplicates: true,
                    });

                    const organizationIds = await makeAChoice(
                        await getOrganizationsChoiceList(true),
                        'Choice organization',
                        'checkbox',
                    );

                    const permissions = [] as PermissionSchema[];
                    for (const organizationId of organizationIds) {
                        for (const type of Object.keys(
                            PermissionTypes,
                        ) as (keyof typeof PermissionTypes)[]) {
                            permissions.push({
                                organizationId,
                                uid: parseInt(uid, 10),
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

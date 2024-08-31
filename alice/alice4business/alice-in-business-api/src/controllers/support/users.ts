import { asyncJsonResponse } from '../utils';
import { Permission, User } from '../../db';
import {
    PermissionSchema,
    Source,
    Types as PermissionTypes,
} from '../../db/tables/permission';
import { UserSchema } from '../../db/tables/user';

import { getUid, logSupportOperation } from './utils';

const OK_STATUS = 'ok'
const NOT_OK_STATUS = 'not ok'

export const createUser = asyncJsonResponse(
    async (req) => {
        const operationType = 'users:create';
        const logPuid = req.user.uid.toString();

        try {
            const { login, organizationIdsCheckbox } = req.body.params

            const uid = await getUid(login);

            await User.bulkCreate([{ id: uid, login }] as UserSchema[], {
                ignoreDuplicates: true,
            });


            const permissions = [] as PermissionSchema[];
            for (const organizationId of organizationIdsCheckbox) {
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

            await logSupportOperation(operationType, true, logPuid)
            return {
                status: OK_STATUS,
            }
        } catch (e) {
            const id = await logSupportOperation(operationType, false, logPuid, e.message)
            return {
                status: NOT_OK_STATUS,
                message: `Operation failed with id ${id}`
            }
        }

    },
    { wrapResult: true },
)

export const bindUsers = asyncJsonResponse(
    async (req) => {
        const operationType = 'users:bind';
        const logPuid = req.user.uid.toString();

        try {
            const { userIdsCheckbox, organizationIdsCheckbox } = req.body.params

            const permissions = [] as PermissionSchema[];
            for (const organizationId of organizationIdsCheckbox) {
                for (const userId of userIdsCheckbox) {
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

            await logSupportOperation(operationType, true, logPuid)
            return {
                status: OK_STATUS,
            }
        } catch (e) {
            const id = await logSupportOperation(operationType, false, logPuid, e.message)
            return {
                status: NOT_OK_STATUS,
                message: `Operation failed with id ${id}`
            }
        }

    },
    { wrapResult: true },
)

export const getAllUsers = asyncJsonResponse(
    async () => {
        const users = await User.findAll();
        return users.map((user) => ({
            value: user.id,
            name: `${user.login} (${user.id})`,
        }));
    },
    { wrapResult: true },
)

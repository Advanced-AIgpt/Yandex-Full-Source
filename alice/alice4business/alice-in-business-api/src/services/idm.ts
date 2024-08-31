import { Message, RoleError } from '../controllers/idm/errors';
import { rolesConfig } from '../controllers/idm/config';
import { Role, User } from '../db';
import { Roles, RoleInstance } from '../db/tables/role';
import { UserInstance } from '../db/tables/user';
import ip from 'ip';

import { getUserInfo } from './blackbox';

export interface RoleManipulateParams {
    login: string;
    role: keyof typeof Roles;
}

const getUserId = async (login: string) => {
    let uid: string;
    try {
        uid = (await getUserInfo({ login, userip: ip.address() })).body.users[0].id;
    } catch {
        throw new RoleError(1, Message.blackboxAccessProblems, 'error');
    }
    if (uid === '') {
        throw new RoleError(1, Message.nonexistPassportLogin, 'fatal');
    } else {
        return uid;
    }
};

export const addRole = async (req: RoleManipulateParams) => {
    const { login, role: idmRole } = req;
    const uid = await getUserId(login);
    const roles = (await Role.findAll({ where: { uid } })).map((role) => role.role)

    await User.findOrCreate({ where: { id: uid }, defaults: { id: uid, login } });

    if (roles.includes(idmRole)) {
        throw new RoleError(0, Message.alreadyHaveRole, 'warning');
    } else {
        try {
            await Role.create({ uid, role: idmRole });
        } catch {
            throw new RoleError(1, Message.databaseProblems, 'error');
        }
    }
};

export const removeRole = async (req: RoleManipulateParams) => {
    const { login , role: idmRole } = req;
    const uid = await getUserId(login);

    const role = await Role.findOne({ where: { uid,  role: idmRole } });

    if (!role){
        throw new RoleError(0, Message.alreadyRemovedRole, 'warning');
    } else {
        try {
            await Role.destroy({ where: { uid,  role: idmRole } })
        } catch {
            throw new RoleError(1, Message.databaseProblems, 'error');
        }
    }
};

export const getAllRoles = async () => {
    try {
        const usersWithPermission = (await Role.findAll({
            include: [
                {
                    model: User,
                    attributes: ['login'],
                    required: true,
                },
            ],
        })) as (RoleInstance & { user: Pick<UserInstance, 'login'> })[];

        const serializedRoles: { uid: string, roles: any; }[] = [];
        usersWithPermission.forEach((user) => {
            const index = serializedRoles.findIndex((el) => el.uid === user.uid);
            if(index !== -1) {
                serializedRoles[index] = {
                    ...serializedRoles[index],
                    roles: [
                        ...serializedRoles[index].roles,
                        {[rolesConfig.slug]: user.role, 'passport-login': user.user.login}
                    ]
                }
            } else {
                serializedRoles.push({
                    uid: user.uid,
                    roles: [{[rolesConfig.slug]: user.role, 'passport-login': user.user.login}]
                })
            }
        });

        return serializedRoles;
    } catch {
        throw new RoleError(1, Message.databaseProblems, 'error');
    }
};

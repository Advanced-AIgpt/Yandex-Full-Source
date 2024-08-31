import test from 'ava';
import uniqueString from 'unique-string';
import { v4 as uuidv4 } from 'uuid';
import * as ACL from '../../../lib/acl';
import { Organization, Permission, User } from '../../../db';
import Sequelize from 'sequelize';
import data from '../../helpers/data';
import { bindUserToOrganization } from '../../helpers/db';
import {
    OrganizationInstance,
    OrganizationSchema,
} from '../../../db/tables/organization';
import RestApiError from '../../../lib/errors/restApi';
import { UserSchema } from '../../../db/tables/user';

test('неизвестный пользователь, неизвестная организация', async (t) => {
    const user = new ACL.User(data.randomUid(), data.user.ip);

    await t.throwsAsync(ACL.getOrganization(user, [], { id: uuidv4() }), {
        instanceOf: Sequelize.EmptyResultError,
    });
});

test('неизвестный пользователь', async (t) => {
    const user = new ACL.User(data.randomUid(), data.user.ip);

    const organization = await Organization.create({
        name: uniqueString(),
    } as OrganizationSchema);
    try {
        await t.throwsAsync(ACL.getOrganization(user, [], { id: organization.id }), {
            instanceOf: Sequelize.EmptyResultError,
        });
    } finally {
        await organization.destroy();
    }
});

test('неизвестная организация', async (t) => {
    const user = new ACL.User(data.randomUid(), data.user.ip);
    const userModel = await User.create({
        id: user.uid.toString(10),
        login: uniqueString(),
    } as UserSchema);
    try {
        await t.throwsAsync(ACL.getOrganization(user, [], { id: uuidv4() }), {
            instanceOf: Sequelize.EmptyResultError,
        });
    } finally {
        await userModel.destroy();
    }
});

test('пользователь не в организации', async (t) => {
    const user = new ACL.User(data.randomUid(), data.user.ip);
    const userModel = await User.create({
        id: user.uid.toString(10),
        login: uniqueString(),
    } as UserSchema);
    try {
        const organization = await Organization.create({
            name: uniqueString(),
        } as OrganizationSchema);
        try {
            await t.throwsAsync(ACL.getOrganization(user, [], { id: organization.id }), {
                instanceOf: Sequelize.EmptyResultError,
            });
        } finally {
            await organization.destroy();
        }
    } finally {
        await userModel.destroy();
    }
});

test('нет нужных прав', async (t) => {
    const user = new ACL.User(data.randomUid(), data.user.ip);
    const userModel = await User.create({
        id: user.uid.toString(10),
        login: uniqueString(),
    } as UserSchema);
    try {
        const organization = await Organization.create({
            name: uniqueString(),
        } as OrganizationSchema);
        try {
            await bindUserToOrganization(user.uid, organization.id, ['status']);

            // проверка с одним видом прав
            await t.throwsAsync(
                ACL.getOrganization(user, ['edit'], { id: organization.id }),
                {
                    instanceOf: RestApiError,
                    message: 'Permission denied',
                },
            );

            // проверка с несколькими правами
            await t.throwsAsync(
                ACL.getOrganization(user, ['edit', 'status'], { id: organization.id }),
                {
                    instanceOf: RestApiError,
                    message: 'Permission denied',
                },
            );
        } finally {
            await organization.destroy();
        }
    } finally {
        await userModel.destroy();
    }
});

test('успешный сценарий', async (t) => {
    const user = new ACL.User(data.randomUid(), data.user.ip);
    const userModel = await User.create({
        id: user.uid.toString(10),
        login: uniqueString(),
    } as UserSchema);
    try {
        const organization = await Organization.create({
            name: uniqueString(),
        } as OrganizationSchema);
        try {
            await bindUserToOrganization(user.uid, organization.id);

            await organization.reload({
                include: [
                    {
                        model: Permission,
                        attributes: ['type'],
                        where: { uid: user.uid },
                    },
                ],
            });

            const actual = await ACL.getOrganization(user, [], { id: organization.id });
            t.deepEqual(extractDataValues(actual), extractDataValues(organization));
        } finally {
            await organization.destroy();
        }
    } finally {
        await userModel.destroy();
    }
});

const extractDataValues = (organization: OrganizationInstance) => {
    const result = organization.get({ clone: true, plain: true });

    (result as OrganizationInstance).permissions?.sort((a, b) =>
        a.type.localeCompare(b.type),
    );

    return result;
};

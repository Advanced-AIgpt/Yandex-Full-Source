import test from 'ava';
import uniqueString from 'unique-string';
import * as ACL from '../../../lib/acl';
import { Organization, Permission, User } from '../../../db';
import data from '../../helpers/data';
import { bindUserToOrganization } from '../../helpers/db';
import {
    OrganizationInstance,
    OrganizationSchema,
} from '../../../db/tables/organization';
import { UserSchema } from '../../../db/tables/user';

test('неизвестный пользователь', async (t) => {
    const user = new ACL.User(data.randomUid(), data.user.ip);

    const organization = await Organization.create({
        name: uniqueString(),
    } as OrganizationSchema);
    try {
        const actual = await ACL.getOrganizations(user, []);
        t.deepEqual(actual, []);
    } finally {
        await organization.destroy();
    }
});

test('нет организаций', async (t) => {
    const user = new ACL.User(data.randomUid(), data.user.ip);
    const userModel = await User.create({
        id: user.uid.toString(10),
        login: uniqueString(),
    } as UserSchema);
    try {
        const actual = await ACL.getOrganizations(user, []);
        t.deepEqual(actual, []);
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
            const actual = await ACL.getOrganizations(user, []);
            t.deepEqual(actual, []);
        } finally {
            await organization.destroy();
        }
    } finally {
        await userModel.destroy();
    }
});

test('проверка прав', async (t) => {
    const user = new ACL.User(data.randomUid(), data.user.ip);
    const userModel = await User.create({
        id: user.uid.toString(10),
        login: uniqueString(),
    } as UserSchema);
    try {
        const organizations = await Organization.bulkCreate([
            { name: uniqueString() } as OrganizationSchema,
            { name: uniqueString() } as OrganizationSchema,
            { name: uniqueString() } as OrganizationSchema,
            { name: uniqueString() } as OrganizationSchema,
        ]);
        try {
            await bindUserToOrganization(user.uid, organizations[0].id, []);
            await bindUserToOrganization(user.uid, organizations[1].id, ['view']);
            await bindUserToOrganization(user.uid, organizations[2].id, ['status']);
            await bindUserToOrganization(user.uid, organizations[3].id, [
                'status',
                'edit',
            ]);
            await Promise.all(
                organizations.map((organization) =>
                    organization.reload({
                        include: [
                            {
                                model: Permission,
                                attributes: ['type'],
                                where: { uid: user.uid },
                                required: false,
                            },
                        ],
                    }),
                ),
            );

            // проверка базового доступа
            const actual1 = await ACL.getOrganizations(user, []);
            t.deepEqual(
                actual1.sort(byId).map(extractDataValues),
                organizations.slice(1).sort(byId).map(extractDataValues),
            );

            // проверка с одним правом
            const actual2 = await ACL.getOrganizations(user, ['status']);
            t.deepEqual(
                actual2.sort(byId).map(extractDataValues),
                organizations.slice(2).sort(byId).map(extractDataValues),
            );

            // проверка с несколькими правами
            const actual3 = await ACL.getOrganizations(user, ['status', 'edit']);
            t.deepEqual(
                actual3.sort(byId).map(extractDataValues),
                organizations.slice(3).sort(byId).map(extractDataValues),
            );

            const actual4 = await ACL.getOrganizations(user, ['promocode']);
            t.deepEqual(actual4, []);
        } finally {
            await Promise.all(
                organizations.map((organization) => organization.destroy()),
            );
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
                        required: false,
                    },
                ],
            });

            const actual = await ACL.getOrganizations(user, []);
            t.deepEqual(
                actual.map(extractDataValues),
                [organization].map(extractDataValues),
            );
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

const byId = (a: OrganizationInstance, b: OrganizationInstance) =>
    a.id.localeCompare(b.id);

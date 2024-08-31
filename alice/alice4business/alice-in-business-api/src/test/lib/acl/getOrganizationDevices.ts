import test from 'ava';
import uniqueString from 'unique-string';
import { v4 as uuidv4 } from 'uuid';
import * as ACL from '../../../lib/acl';
import { Device, Organization, User } from '../../../db';
import { DeviceInstance, DeviceSchema, Platform } from '../../../db/tables/device';
import Sequelize from 'sequelize';
import data from '../../helpers/data';
import { bindUserToOrganization } from '../../helpers/db';
import RestApiError from '../../../lib/errors/restApi';
import { OrganizationSchema } from '../../../db/tables/organization';
import { UserSchema } from '../../../db/tables/user';

test('неизвестный пользователь, неизвестная организация', async (t) => {
    const user = new ACL.User(data.randomUid(), data.user.ip);

    await t.throwsAsync(ACL.getOrganizationDevices(user, [], uuidv4()), {
        instanceOf: Sequelize.EmptyResultError,
    });
});

test('неизвестный пользователь', async (t) => {
    const user = new ACL.User(data.randomUid(), data.user.ip);

    const organization = await Organization.create({
        name: uniqueString(),
    } as OrganizationSchema);
    try {
        await t.throwsAsync(ACL.getOrganizationDevices(user, [], organization.id), {
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
        await t.throwsAsync(ACL.getOrganizationDevices(user, [], uuidv4()), {
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
            await Device.bulkCreate([
                {
                    organizationId: organization.id,

                    platform: Platform.YandexStation,
                    deviceId: uniqueString(),

                    externalDeviceId: uniqueString(),
                } as DeviceSchema,
                {
                    organizationId: organization.id,

                    platform: Platform.YandexStation,
                    deviceId: uniqueString(),

                    externalDeviceId: uniqueString(),
                } as DeviceSchema,
            ]);

            await t.throwsAsync(ACL.getOrganizationDevices(user, [], organization.id), {
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

            const devices = await Device.bulkCreate([
                {
                    organizationId: organization.id,

                    platform: Platform.YandexStation,
                    deviceId: uniqueString(),

                    kolonkishId: uniqueString(),

                    externalDeviceId: uniqueString(),
                } as DeviceSchema,
                {
                    organizationId: organization.id,

                    platform: Platform.YandexStation,
                    deviceId: uniqueString(),

                    externalDeviceId: uniqueString(),
                } as DeviceSchema,
            ]);
            await Promise.all(devices.map((device) => device.reload()));

            // проверка с одним видом прав
            await t.throwsAsync(
                ACL.getOrganizationDevices(user, ['edit'], organization.id),
                {
                    instanceOf: RestApiError,
                    message: 'Permission denied',
                },
            );

            // проверка с несколькими правами
            await t.throwsAsync(
                ACL.getOrganizationDevices(user, ['edit', 'status'], organization.id),
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

test('успешный сценарий: в организации нет устройств', async (t) => {
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

            const actual = await ACL.getOrganizationDevices(user, [], organization.id);
            t.deepEqual(actual, []);
        } finally {
            await organization.destroy();
        }
    } finally {
        await userModel.destroy();
    }
});

test('успешный сценарий: устройства есть', async (t) => {
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

            const devices = await Device.bulkCreate([
                {
                    organizationId: organization.id,

                    platform: Platform.YandexStation,
                    deviceId: uniqueString(),

                    kolonkishId: uniqueString(),

                    externalDeviceId: uniqueString(),
                } as DeviceSchema,
                {
                    organizationId: organization.id,

                    platform: Platform.YandexStation,
                    deviceId: uniqueString(),

                    externalDeviceId: uniqueString(),
                } as DeviceSchema,
            ]);
            await Promise.all(devices.map((device) => device.reload()));

            const actual = await ACL.getOrganizationDevices(user, [], organization.id);
            t.deepEqual(
                actual.sort(byId).map(extractDataValues),
                devices.slice(0).sort(byId).map(extractDataValues),
            );
        } finally {
            await organization.destroy();
        }
    } finally {
        await userModel.destroy();
    }
});

test('успешный сценарий: устройства удалены', async (t) => {
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

            const devices = await Device.bulkCreate([
                {
                    organizationId: organization.id,

                    platform: Platform.YandexStation,
                    deviceId: uniqueString(),

                    kolonkishId: uniqueString(),

                    externalDeviceId: uniqueString(),
                } as DeviceSchema,
                {
                    organizationId: organization.id,

                    platform: Platform.YandexStation,
                    deviceId: uniqueString(),

                    externalDeviceId: uniqueString(),
                } as DeviceSchema,
            ]);
            await Promise.all(devices.map((device) => device.reload()));

            await devices[0].destroy();

            const actual = await ACL.getOrganizationDevices(user, [], organization.id);
            t.deepEqual(
                actual.sort(byId).map(extractDataValues),
                devices.slice(1).sort(byId).map(extractDataValues),
            );

            const actualWithRemoved = await ACL.getOrganizationDevices(
                user,
                [],
                organization.id,
                null,
                {
                    paranoid: false,
                },
            );
            t.deepEqual(
                actualWithRemoved.sort(byId).map(extractDataValues),
                devices.slice(0).sort(byId).map(extractDataValues),
            );
        } finally {
            await organization.destroy();
        }
    } finally {
        await userModel.destroy();
    }
});

const extractDataValues = (device: DeviceInstance) => {
    const result = device.get({ clone: true, plain: true });

    (result as DeviceInstance).permissions?.sort((a, b) => a.type.localeCompare(b.type));

    return result;
};

const byId = (a: DeviceInstance, b: DeviceInstance) => a.id.localeCompare(b.id);

import test from 'ava';
import uniqueString from 'unique-string';
import { v4 as uuidv4 } from 'uuid';
import * as ACL from '../../../lib/acl';
import { Device, Organization, Permission, User } from '../../../db';
import { DeviceInstance, DeviceSchema, Platform } from '../../../db/tables/device';
import Sequelize from 'sequelize';
import data from '../../helpers/data';
import { bindUserToOrganization } from '../../helpers/db';
import RestApiError from '../../../lib/errors/restApi';
import { OrganizationSchema } from '../../../db/tables/organization';
import { UserSchema } from '../../../db/tables/user';

test('неизвестный пользователь, неизвестное устройство', async (t) => {
    const user = new ACL.User(data.randomUid(), data.user.ip);

    const id = uuidv4();
    await t.throwsAsync(ACL.getDevice(user, [], { id }), {
        instanceOf: Sequelize.EmptyResultError,
    });

    const deviceId = uniqueString();
    await t.throwsAsync(ACL.getDevice(user, [], { deviceId }), {
        instanceOf: Sequelize.EmptyResultError,
    });

    const externalDeviceId = uniqueString();
    await t.throwsAsync(ACL.getDevice(user, [], { externalDeviceId }), {
        instanceOf: Sequelize.EmptyResultError,
    });
});

test('неизвестный пользователь', async (t) => {
    const user = new ACL.User(data.randomUid(), data.user.ip);

    const organization = await Organization.create({
        name: uniqueString(),
    } as OrganizationSchema);
    try {
        const device = await Device.create({
            organizationId: organization.id,

            platform: Platform.YandexStation,
            deviceId: uniqueString(),

            externalDeviceId: uniqueString(),
        } as DeviceSchema);

        await t.throwsAsync(ACL.getDevice(user, [], { id: device.id }), {
            instanceOf: Sequelize.EmptyResultError,
        });

        await t.throwsAsync(ACL.getDevice(user, [], { deviceId: device.deviceId }), {
            instanceOf: Sequelize.EmptyResultError,
        });

        await t.throwsAsync(
            ACL.getDevice(user, [], { externalDeviceId: device.externalDeviceId! }),
            {
                instanceOf: Sequelize.EmptyResultError,
            },
        );
    } finally {
        await organization.destroy();
    }
});

test('неизвестное устройство', async (t) => {
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

            const id = uuidv4();
            await t.throwsAsync(ACL.getDevice(user, [], { id }), {
                instanceOf: Sequelize.EmptyResultError,
            });

            const deviceId = uniqueString();
            await t.throwsAsync(ACL.getDevice(user, [], { deviceId }), {
                instanceOf: Sequelize.EmptyResultError,
            });

            const externalDeviceId = uniqueString();
            await t.throwsAsync(ACL.getDevice(user, [], { externalDeviceId }), {
                instanceOf: Sequelize.EmptyResultError,
            });
        } finally {
            await organization.destroy();
        }
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
            const device = await Device.create({
                organizationId: organization.id,

                platform: Platform.YandexStation,
                deviceId: uniqueString(),

                externalDeviceId: uniqueString(),
            } as DeviceSchema);

            await t.throwsAsync(ACL.getDevice(user, [], { id: device.id }), {
                instanceOf: Sequelize.EmptyResultError,
            });

            await t.throwsAsync(ACL.getDevice(user, [], { deviceId: device.deviceId }), {
                instanceOf: Sequelize.EmptyResultError,
            });

            await t.throwsAsync(
                ACL.getDevice(user, [], { externalDeviceId: device.externalDeviceId! }),
                {
                    instanceOf: Sequelize.EmptyResultError,
                },
            );
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

            const device = await Device.create({
                organizationId: organization.id,

                platform: Platform.YandexStation,
                deviceId: uniqueString(),

                externalDeviceId: uniqueString(),
            } as DeviceSchema);

            // проверка с одним видом прав
            await t.throwsAsync(ACL.getDevice(user, ['edit'], { id: device.id }), {
                instanceOf: RestApiError,
                message: 'Permission denied',
            });

            await t.throwsAsync(
                ACL.getDevice(user, ['edit'], { deviceId: device.deviceId }),
                {
                    instanceOf: RestApiError,
                    message: 'Permission denied',
                },
            );

            await t.throwsAsync(
                ACL.getDevice(user, ['edit'], {
                    externalDeviceId: device.externalDeviceId!,
                }),
                {
                    instanceOf: RestApiError,
                    message: 'Permission denied',
                },
            );

            // проверка с несколькими правами
            await t.throwsAsync(
                ACL.getDevice(user, ['edit', 'status'], { id: device.id }),
                {
                    instanceOf: RestApiError,
                    message: 'Permission denied',
                },
            );

            await t.throwsAsync(
                ACL.getDevice(user, ['edit', 'status'], { deviceId: device.deviceId }),
                {
                    instanceOf: RestApiError,
                    message: 'Permission denied',
                },
            );

            await t.throwsAsync(
                ACL.getDevice(user, ['edit', 'status'], {
                    externalDeviceId: device.externalDeviceId!,
                }),
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

test('устройство удалено', async (t) => {
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
            const device = await Device.create({
                organizationId: organization.id,

                platform: Platform.YandexStation,
                deviceId: uniqueString(),

                externalDeviceId: uniqueString(),
            } as DeviceSchema);
            await device.reload({
                include: [
                    {
                        model: Permission,
                        attributes: ['type'],
                        where: { uid: user.uid },
                        required: false,
                    },
                ],
            });
            await device.destroy();

            // check paranoid=false
            const actual1 = await ACL.getDevice(
                user,
                [],
                { id: device.id },
                { paranoid: false },
            );
            t.deepEqual(extractDataValues(actual1), extractDataValues(device));

            const actual2 = await ACL.getDevice(
                user,
                [],
                { deviceId: device.deviceId },
                { paranoid: false },
            );
            t.deepEqual(extractDataValues(actual2), extractDataValues(device));

            const actual3 = await ACL.getDevice(
                user,
                [],
                {
                    externalDeviceId: device.externalDeviceId!,
                },
                { paranoid: false },
            );
            t.deepEqual(extractDataValues(actual3), extractDataValues(device));

            // check default paranoid mode
            await t.throwsAsync(ACL.getDevice(user, [], { id: device.id }), {
                instanceOf: Sequelize.EmptyResultError,
            });

            await t.throwsAsync(ACL.getDevice(user, [], { deviceId: device.deviceId }), {
                instanceOf: Sequelize.EmptyResultError,
            });

            await t.throwsAsync(
                ACL.getDevice(user, [], { externalDeviceId: device.externalDeviceId! }),
                {
                    instanceOf: Sequelize.EmptyResultError,
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
            const device = await Device.create({
                organizationId: organization.id,

                platform: Platform.YandexStation,
                deviceId: uniqueString(),

                externalDeviceId: uniqueString(),
            } as DeviceSchema);
            await device.reload({
                include: [
                    {
                        model: Permission,
                        attributes: ['type'],
                        where: { uid: user.uid },
                        required: false,
                    },
                ],
            });

            const actual1 = await ACL.getDevice(user, [], { id: device.id });
            t.deepEqual(extractDataValues(actual1), extractDataValues(device));

            const actual2 = await ACL.getDevice(user, [], { deviceId: device.deviceId });
            t.deepEqual(extractDataValues(actual2), extractDataValues(device));

            const actual3 = await ACL.getDevice(user, [], {
                externalDeviceId: device.externalDeviceId!,
            });
            t.deepEqual(extractDataValues(actual3), extractDataValues(device));
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

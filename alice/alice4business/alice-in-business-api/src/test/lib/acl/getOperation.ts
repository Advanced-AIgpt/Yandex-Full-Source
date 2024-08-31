import test from 'ava';
import uniqueString from 'unique-string';
import { v4 as uuidv4 } from 'uuid';
import * as ACL from '../../../lib/acl';
import { Device, Operation, Organization, Permission, User } from '../../../db';
import { DeviceSchema, Platform } from '../../../db/tables/device';
import {
    OperationInstance,
    OperationSchema,
    Status as OperationStatus,
    Type as OperationType,
} from '../../../db/tables/operation';
import Sequelize from 'sequelize';
import data from '../../helpers/data';
import { bindUserToOrganization } from '../../helpers/db';
import RestApiError from '../../../lib/errors/restApi';
import { OrganizationSchema } from '../../../db/tables/organization';
import { UserSchema } from '../../../db/tables/user';

test('неизвестный пользователь, неизвестная операция', async (t) => {
    const user = new ACL.User(data.randomUid(), data.user.ip);

    await t.throwsAsync(ACL.getOperation(user, [], { id: uuidv4() }), {
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

        const operation = await Operation.create({
            devicePk: device.id,
            type: OperationType.Activate,
            status: OperationStatus.Rejected,
        } as OperationSchema);

        await t.throwsAsync(ACL.getOperation(user, [], { id: operation.id }), {
            instanceOf: Sequelize.EmptyResultError,
        });
    } finally {
        await organization.destroy();
    }
});

test('неизвестная операция', async (t) => {
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

            await t.throwsAsync(ACL.getOperation(user, [], { id: uuidv4() }), {
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
            const operation = await Operation.create({
                devicePk: device.id,
                type: OperationType.Activate,
                status: OperationStatus.Rejected,
            } as OperationSchema);

            await t.throwsAsync(ACL.getOperation(user, [], { id: operation.id }), {
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
            const device = await Device.create({
                organizationId: organization.id,

                platform: Platform.YandexStation,
                deviceId: uniqueString(),

                externalDeviceId: uniqueString(),
            } as DeviceSchema);
            const operation = await Operation.create({
                devicePk: device.id,
                type: OperationType.Activate,
                status: OperationStatus.Rejected,
            } as OperationSchema);

            // проверка с одним видом прав
            await t.throwsAsync(
                ACL.getOperation(user, ['edit'], { id: operation.id }),
                {
                    instanceOf: RestApiError,
                    message: 'Permission denied',
                }
            );

            // проверка с несколькими правами
            await t.throwsAsync(
                ACL.getOperation(user, ['edit', 'status'], { id: operation.id }),
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

            const device = await Device.create({
                organizationId: organization.id,

                platform: Platform.YandexStation,
                deviceId: uniqueString(),

                externalDeviceId: uniqueString(),
            } as DeviceSchema);
            const operation = await Operation.create({
                devicePk: device.id,
                type: OperationType.Activate,
                status: OperationStatus.Rejected,
            } as OperationSchema);
            await operation.reload({
                include: [
                    {
                        model: Device,
                        paranoid: false,
                        include: [
                            {
                                model: Permission,
                                attributes: ['type'],
                                where: { uid: user.uid },
                            },
                        ],
                    },
                ],
            });

            const actual = await ACL.getOperation(user, [], { id: operation.id });
            t.deepEqual(extractDataValues(actual), extractDataValues(operation));
        } finally {
            await organization.destroy();
        }
    } finally {
        await userModel.destroy();
    }
});

test('успешный сценарий + устройство удалено', async (t) => {
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
            await device.destroy();

            const operation = await Operation.create({
                devicePk: device.id,
                type: OperationType.Activate,
                status: OperationStatus.Rejected,
            } as OperationSchema);
            await operation.reload({
                include: [
                    {
                        model: Device,
                        paranoid: false,
                        include: [
                            {
                                model: Permission,
                                attributes: ['type'],
                                where: { uid: user.uid },
                            },
                        ],
                    },
                ],
            });

            const actual = await ACL.getOperation(user, [], { id: operation.id });
            t.deepEqual(extractDataValues(actual), extractDataValues(operation));
        } finally {
            await organization.destroy();
        }
    } finally {
        await userModel.destroy();
    }
});

const extractDataValues = (operation: OperationInstance) => {
    const result = operation.get({ clone: true, plain: true });

    (result as OperationInstance).device?.permissions?.sort((a, b) =>
        a.type.localeCompare(b.type),
    );

    return result;
};

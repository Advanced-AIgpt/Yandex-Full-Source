import anyTest, { TestInterface } from 'ava';
import sinon from 'sinon';
import uniqueString from 'unique-string';
import { ActivationCode, Device, Organization } from '../../../db';
import {
    OrganizationInstance,
    OrganizationSchema,
} from '../../../db/tables/organization';
import {
    DeviceInstance,
    DeviceSchema,
    Platform,
    Status,
} from '../../../db/tables/device';
import { callInternalDialogovoApi, responds, respondsWithError } from '../../helpers/api';
import Sequelize from 'sequelize';
import config from '../../../lib/config';
import { ActivationCodeSchema } from '../../../db/tables/activationCode';

interface Context {
    organization: OrganizationInstance;
    device: DeviceInstance;
}
const test = anyTest as TestInterface<Context>;

test.beforeEach(async (t) => {
    t.context.organization = await Organization.create({
        name: `${t.title} ${uniqueString()}`,
    } as OrganizationSchema);

    t.context.device = await Device.create({
        organizationId: t.context.organization.id,
        platform: Object.values(Platform)[
            Math.floor(Object.values(Platform).length * Math.random())
        ],
        deviceId: uniqueString(),
    } as DeviceSchema);
});

test.afterEach.always(async (t) => {
    t.context.device?.id &&
        (await ActivationCode.destroy({
            where: { deviceId: t.context.device.id },
        }));
    await t.context.organization.destroy({ force: true });
    sinon.restore();
});

test('Не переданы параметры', async (t) => {
    await respondsWithError({
        res: await callInternalDialogovoApi('get', '/device_state'),
        code: 400,
        message: 'Query params "platform" and "device_id" are required',
        t,
    });

    await respondsWithError({
        res: await callInternalDialogovoApi('get', '/device_state', {
            query: { platform: uniqueString() },
        }),
        code: 400,
        message: 'Query params "platform" and "device_id" are required',
        t,
    });

    await respondsWithError({
        res: await callInternalDialogovoApi('get', '/device_state', {
            query: { device_id: uniqueString() },
        }),
        code: 400,
        message: 'Query params "platform" and "device_id" are required',
        t,
    });
});

test('Сломана БД', async (t) => {
    sinon
        .stub(Device, 'findOne')
        .rejects(new Sequelize.ConnectionRefusedError(new Error()));

    await respondsWithError({
        res: await callInternalDialogovoApi('get', '/device_state', {
            query: {
                platform: uniqueString(),
                device_id: uniqueString(),
            },
        }),
        code: 500,
        message: 'Unexpected error',
        t,
    });
});

test('Неизвестное устройство', async (t) => {
    await respondsWithError({
        res: await callInternalDialogovoApi('get', '/device_state', {
            query: {
                platform: uniqueString(),
                device_id: uniqueString(),
            },
        }),
        code: 404,
        message: 'Device not found',
        t,
    });
});

test('Устройство удалено', async (t) => {
    await t.context.device.destroy();

    await respondsWithError({
        res: await callInternalDialogovoApi('get', '/device_state', {
            query: {
                platform: t.context.device.platform,
                device_id: t.context.device.deviceId,
            },
        }),
        code: 404,
        message: 'Device not found',
        t,
    });
});

test('status = reset (в базе нет кода)', async (t) => {
    await t.context.device.update({
        status: Status.Reset,
    } as DeviceSchema);
    await ActivationCode.destroy({
        where: {
            deviceId: t.context.device.id,
        },
    });

    const res = await callInternalDialogovoApi('get', '/device_state', {
        query: {
            platform: t.context.device.platform,
            device_id: t.context.device.deviceId,
        },
    });

    const codes = await ActivationCode.findAll({
        where: { deviceId: t.context.device.id },
        order: [['createdAt', 'DESC']],
    });
    t.is(codes.length, 1);

    const code = `${codes[0].code.substr(0, 3)} ${codes[0].code.substr(
        3,
        3,
    )} ${codes[0].code.substr(6, 3)}`;
    await responds({
        res,
        code: 200,
        wrapResult: true,
        expect: {
            locked: true,
            outputSpeech: `Устройство заблокировано. Для активации откройте страницу ya.cc/activate (но это не точно) и введите код ${code}`,
            mordoviaUrl: `${config.dialogovo.mordoviaUrl}?code=${codes[0].code}`,
            stationUrl: config.dialogovo.mordoviaUrl,
            userUrl: 'http://ya.cc',
            code: codes[0].code,
        },
        t,
    });
});

test('status = reset (в базе есть старый код)', async (t) => {
    await t.context.device.update({
        status: Status.Reset,
    } as DeviceSchema);
    const outdatedCode = await ActivationCode.create({
        code: Math.random().toFixed(9).substr(2),
        deviceId: t.context.device.id,
        createdAt: new Date(
            Date.now() - config.app.customerActivationOperation.codeRefreshInterval,
        ),
    } as ActivationCodeSchema);

    const res = await callInternalDialogovoApi('get', '/device_state', {
        query: {
            platform: t.context.device.platform,
            device_id: t.context.device.deviceId,
        },
    });

    const codes = await ActivationCode.findAll({
        where: { deviceId: t.context.device.id },
        order: [['createdAt', 'DESC']],
    });
    t.is(codes.length, 2);
    t.not(codes[0].code, outdatedCode.code);

    const code = `${codes[0].code.substr(0, 3)} ${codes[0].code.substr(
        3,
        3,
    )} ${codes[0].code.substr(6, 3)}`;
    await responds({
        res,
        code: 200,
        wrapResult: true,
        expect: {
            locked: true,
            outputSpeech: `Устройство заблокировано. Для активации откройте страницу ya.cc/activate (но это не точно) и введите код ${code}`,
            mordoviaUrl: `${config.dialogovo.mordoviaUrl}?code=${codes[0].code}`,
            stationUrl: config.dialogovo.mordoviaUrl,
            userUrl: 'http://ya.cc',
            code: codes[0].code,
        },
        t,
    });
});

test('status = reset (в базе есть свежий код)', async (t) => {
    await t.context.device.update({
        status: Status.Reset,
    } as DeviceSchema);
    const actualCode = await ActivationCode.create({
        code: Math.random().toFixed(9).substr(2),
        deviceId: t.context.device.id,
        createdAt: new Date(),
    } as ActivationCodeSchema);

    const res = await callInternalDialogovoApi('get', '/device_state', {
        query: {
            platform: t.context.device.platform,
            device_id: t.context.device.deviceId,
        },
    });

    const codes = await ActivationCode.findAll({
        where: { deviceId: t.context.device.id },
        order: [['createdAt', 'DESC']],
    });
    t.is(codes.length, 1);

    const code = `${actualCode.code.substr(0, 3)} ${actualCode.code.substr(
        3,
        3,
    )} ${actualCode.code.substr(6, 3)}`;
    await responds({
        res,
        code: 200,
        wrapResult: true,
        expect: {
            locked: true,
            outputSpeech: `Устройство заблокировано. Для активации откройте страницу ya.cc/activate (но это не точно) и введите код ${code}`,
            mordoviaUrl: `${config.dialogovo.mordoviaUrl}?code=${actualCode.code}`,
            stationUrl: config.dialogovo.mordoviaUrl,
            userUrl: 'http://ya.cc',
            code: actualCode.code,
        },
        t,
    });
});

test('status = inactive (в базе нет кода)', async (t) => {
    await t.context.device.update({
        status: Status.Inactive,
    } as DeviceSchema);
    await ActivationCode.destroy({
        where: {
            deviceId: t.context.device.id,
        },
    });

    const res = await callInternalDialogovoApi('get', '/device_state', {
        query: {
            platform: t.context.device.platform,
            device_id: t.context.device.deviceId,
        },
    });

    const codes = await ActivationCode.findAll({
        where: { deviceId: t.context.device.id },
        order: [['createdAt', 'DESC']],
    });
    t.is(codes.length, 1);

    const code = `${codes[0].code.substr(0, 3)} ${codes[0].code.substr(
        3,
        3,
    )} ${codes[0].code.substr(6, 3)}`;
    await responds({
        res,
        code: 200,
        wrapResult: true,
        expect: {
            locked: true,
            outputSpeech: `Устройство заблокировано. Для активации откройте страницу ya.cc/activate (но это не точно) и введите код ${code}`,
            mordoviaUrl: `${config.dialogovo.mordoviaUrl}?code=${codes[0].code}`,
            stationUrl: config.dialogovo.mordoviaUrl,
            userUrl: 'http://ya.cc',
            code: codes[0].code,
        },
        t,
    });
});

test('status = inactive (в базе есть старый код)', async (t) => {
    await t.context.device.update({
        status: Status.Inactive,
    } as DeviceSchema);
    const outdatedCode = await ActivationCode.create({
        code: Math.random().toFixed(9).substr(2),
        deviceId: t.context.device.id,
        createdAt: new Date(
            Date.now() - config.app.customerActivationOperation.codeRefreshInterval,
        ),
    } as ActivationCodeSchema);

    const res = await callInternalDialogovoApi('get', '/device_state', {
        query: {
            platform: t.context.device.platform,
            device_id: t.context.device.deviceId,
        },
    });

    const codes = await ActivationCode.findAll({
        where: { deviceId: t.context.device.id },
        order: [['createdAt', 'DESC']],
    });
    t.is(codes.length, 2);
    t.not(codes[0].code, outdatedCode.code);

    const code = `${codes[0].code.substr(0, 3)} ${codes[0].code.substr(
        3,
        3,
    )} ${codes[0].code.substr(6, 3)}`;
    await responds({
        res,
        code: 200,
        wrapResult: true,
        expect: {
            locked: true,
            outputSpeech: `Устройство заблокировано. Для активации откройте страницу ya.cc/activate (но это не точно) и введите код ${code}`,
            mordoviaUrl: `${config.dialogovo.mordoviaUrl}?code=${codes[0].code}`,
            stationUrl: config.dialogovo.mordoviaUrl,
            userUrl: 'http://ya.cc',
            code: codes[0].code,
        },
        t,
    });
});

test('status = inactive (в базе есть свежий код)', async (t) => {
    await t.context.device.update({
        status: Status.Inactive,
    } as DeviceSchema);
    const actualCode = await ActivationCode.create({
        code: Math.random().toFixed(9).substr(2),
        deviceId: t.context.device.id,
        createdAt: new Date(),
    } as ActivationCodeSchema);

    const res = await callInternalDialogovoApi('get', '/device_state', {
        query: {
            platform: t.context.device.platform,
            device_id: t.context.device.deviceId,
        },
    });

    const codes = await ActivationCode.findAll({
        where: { deviceId: t.context.device.id },
        order: [['createdAt', 'DESC']],
    });
    t.is(codes.length, 1);

    const code = `${actualCode.code.substr(0, 3)} ${actualCode.code.substr(
        3,
        3,
    )} ${actualCode.code.substr(6, 3)}`;
    await responds({
        res,
        code: 200,
        wrapResult: true,
        expect: {
            locked: true,
            outputSpeech: `Устройство заблокировано. Для активации откройте страницу ya.cc/activate (но это не точно) и введите код ${code}`,
            mordoviaUrl: `${config.dialogovo.mordoviaUrl}?code=${actualCode.code}`,
            stationUrl: config.dialogovo.mordoviaUrl,
            userUrl: 'http://ya.cc',
            code: actualCode.code,
        },
        t,
    });
});

test('status = active', async (t) => {
    await t.context.device.update({
        status: Status.Active,
    } as DeviceSchema);

    await responds({
        res: await callInternalDialogovoApi('get', '/device_state', {
            query: {
                platform: t.context.device.platform,
                device_id: t.context.device.deviceId,
            },
        }),
        code: 200,
        wrapResult: true,
        expect: {
            locked: false,
        },
        t,
    });
});

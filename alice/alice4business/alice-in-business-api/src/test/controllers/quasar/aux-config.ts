import anyTest, { TestInterface } from 'ava';
import sinon from 'sinon';
import uniqueString from 'unique-string';
import { v4 as uuidv4 } from 'uuid';
import {
    OrganizationInstance,
    OrganizationSchema,
} from '../../../db/tables/organization';
import { DeviceInstance, Platform, Status } from '../../../db/tables/device';
import { Device, Organization } from '../../../db';
import { callInternalQuasarApi, responds } from '../../helpers/api';
import { createDevice } from '../../helpers/db';
import data from '../../helpers/data';

interface Context {
    organization: OrganizationInstance;
    organizationWithSkills: OrganizationInstance;
    device?: DeviceInstance;
}

const test = anyTest as TestInterface<Context>;

test.beforeEach(async (t) => {
    t.context.organization = await Organization.create({
        id: uuidv4(),
        name: `${t.title} ${uniqueString()}`,
    } as OrganizationSchema);

    t.context.organizationWithSkills = await Organization.create({
        id: uuidv4(),
        name: `${t.title} ${uniqueString()}`,
        preactivatedSkillIds: ['abc', 'foo'],
    } as OrganizationSchema);
});
test.afterEach.always(async (t) => {
    sinon.restore();

    if (t.context.device) {
        await t.context.device.destroy({ force: true });
    }
    await t.context.organization.destroy({ force: true });
});

test('Ошибка БД', async (t) => {
    sinon.stub(Device, 'findOne').rejects(new Error('aaaaaa'));

    const res = await callInternalQuasarApi('get', '/aux_config', {
        query: { device_id: uniqueString(), platform: Platform.YandexStation },
    });

    responds({
        t,
        res,
        code: 500,
        expect: {
            status: 'error',
            error: {
                code: 500,
                message: 'Internal error',
            },
        },
        wrapResult: false,
    });
});

test('Пустой запрос', async (t) => {
    sinon.stub(Device, 'findOne').rejects(new Error('aaaaaa'));

    responds({
        t,
        res: await callInternalQuasarApi('get', '/aux_config', {
            query: {},
        }),
        code: 400,
        expect: {
            status: 'error',
            error: {
                code: 400,
                message: 'Query params "platform" and "device_id" are required',
            },
        },
        wrapResult: false,
    });

    responds({
        t,
        res: await callInternalQuasarApi('get', '/aux_config', {
            query: { device_id: uniqueString() },
        }),
        code: 400,
        expect: {
            status: 'error',
            error: {
                code: 400,
                message: 'Query params "platform" and "device_id" are required',
            },
        },
        wrapResult: false,
    });

    responds({
        t,
        res: await callInternalQuasarApi('get', '/aux_config', {
            query: { platform: Platform.YandexStation },
        }),
        code: 400,
        expect: {
            status: 'error',
            error: {
                code: 400,
                message: 'Query params "platform" and "device_id" are required',
            },
        },
        wrapResult: false,
    });
});

test('Неизвестное устройство', async (t) => {
    const res = await callInternalQuasarApi('get', '/aux_config', {
        query: { device_id: uniqueString(), platform: Platform.YandexStation },
    });

    responds({
        t,
        res,
        code: 404,
        expect: {
            status: 'error',
            error: {
                code: 404,
                message: 'Device not found',
            },
        },
        wrapResult: false,
    });
});

test('пустой конфиг', async (t) => {
    t.context.device = await createDevice({
        organizationId: t.context.organization.id,
        status: Status.Reset,
    });

    const now = new Date('2020-01-01T12:00:00Z').valueOf();
    sinon.stub(Date, 'now').returns(now);

    const res = await callInternalQuasarApi('get', '/aux_config', {
        query: {
            device_id: t.context.device.deviceId,
            platform: t.context.device.platform,
        },
    });

    responds({
        t,
        res,
        code: 200,
        expect: {
            result: {
                data: {
                    preactivated_skill_ids: [],
                    unlocked: false
                },
                expires: '2020-01-01T12:15:00.000Z',
            },
        },
        wrapResult: false,
    });
});

test('smartHomeUid is filled', async (t) => {
    const shUid = data.randomUid();

    t.context.device = await createDevice({
        organizationId: t.context.organization.id,
        status: Status.Reset,
        smartHomeUid: shUid,
    });

    const now = new Date('2020-01-01T12:00:00Z').valueOf();
    sinon.stub(Date, 'now').returns(now);

    const res = await callInternalQuasarApi('get', '/aux_config', {
        query: {
            device_id: t.context.device.deviceId,
            platform: t.context.device.platform,
        },
    });

    responds({
        t,
        res,
        code: 200,
        expect: {
            result: {
                data: {
                    preactivated_skill_ids: [],
                    // actually, value is a `string` due to PG module. It's fine, because clients supports this behavior
                    smart_home_uid: shUid.toString(10),
                    unlocked: false
                },
                expires: '2020-01-01T12:15:00.000Z',
            },
        },
        wrapResult: false,
    });
});

test('preactivatedSkillIds is filled', async (t) => {
    const shUid = data.randomUid();

    t.context.device = await createDevice({
        organizationId: t.context.organization.id,
        status: Status.Reset,
        preactivatedSkillIds: ['foo', 'bar'],
    });

    const now = new Date('2020-01-01T12:00:00Z').valueOf();
    sinon.stub(Date, 'now').returns(now);

    const res = await callInternalQuasarApi('get', '/aux_config', {
        query: {
            device_id: t.context.device.deviceId,
            platform: t.context.device.platform,
        },
    });

    responds({
        t,
        res,
        code: 200,
        expect: {
            result: {
                data: {
                    preactivated_skill_ids: ['foo', 'bar'],
                    unlocked: false
                },
                expires: '2020-01-01T12:15:00.000Z',
            },
        },
        wrapResult: false,
    });
});

test('preactivatedSkillIds is filled on organization', async (t) => {
    const shUid = data.randomUid();

    t.context.device = await createDevice({
        organizationId: t.context.organizationWithSkills.id,
        status: Status.Active,
        preactivatedSkillIds: ['foo', 'bar'],
    });

    const now = new Date('2020-01-01T12:00:00Z').valueOf();
    sinon.stub(Date, 'now').returns(now);

    const res = await callInternalQuasarApi('get', '/aux_config', {
        query: {
            device_id: t.context.device.deviceId,
            platform: t.context.device.platform,
        },
    });

    responds({
        t,
        res,
        code: 200,
        expect: {
            result: {
                data: {
                    preactivated_skill_ids: ['abc', 'foo', 'bar'],
                    unlocked: true
                },
                expires: '2020-01-01T12:15:00.000Z',
            },
        },
        wrapResult: false,
    });
});

test('unlocked field correct according to status', async (t) => {
    const shUid = data.randomUid();

    t.context.device = await createDevice({
        organizationId: t.context.organization.id,
        status: Status.Active
    });

    const now = new Date('2020-01-01T12:00:00Z').valueOf();
    sinon.stub(Date, 'now').returns(now);

    const res = await callInternalQuasarApi('get', '/aux_config', {
        query: {
            device_id: t.context.device.deviceId,
            platform: t.context.device.platform,
        },
    });

    responds({
        t,
        res,
        code: 200,
        expect: {
            result: {
                data: {
                    preactivated_skill_ids: [],
                    unlocked: true
                },
                expires: '2020-01-01T12:15:00.000Z',
            },
        },
        wrapResult: false,
    });
});

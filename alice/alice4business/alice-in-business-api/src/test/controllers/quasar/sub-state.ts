import anyTest, { TestInterface } from 'ava';
import sinon from 'sinon';
import uniqueString from 'unique-string';
import { v4 as uuidv4 } from 'uuid';
import { Device, Organization } from '../../../db';
import {
    OrganizationInstance,
    OrganizationSchema,
} from '../../../db/tables/organization';
import { callInternalQuasarApi, responds } from '../../helpers/api';
import { DeviceInstance, Platform, Status } from '../../../db/tables/device';
import { createDevice } from '../../helpers/db';

interface Context {
    organization: OrganizationInstance;
    device?: DeviceInstance;
}
const test = anyTest as TestInterface<Context>;

test.beforeEach(async (t) => {
    t.context.organization = await Organization.create({
        id: uuidv4(),
        name: `${t.title} ${uniqueString()}`,
    } as OrganizationSchema);
});
test.afterEach.always(async (t) => {
    sinon.restore();

    if (t.context.device) {
        await t.context.device.destroy();
    }
    await t.context.organization.destroy();
});

test('Ошибка БД', async (t) => {
    sinon.stub(Device, 'findOne').rejects(new Error('aaaaaa'));

    const res = await callInternalQuasarApi('get', '/sub_state', {
        query: { id: uniqueString(), platform: Platform.YandexStation },
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

    const now = new Date('2020-01-01T12:00:00Z').valueOf();
    sinon.stub(Date, 'now').returns(now);

    responds({
        t,
        res: await callInternalQuasarApi('get', '/sub_state', {
            query: {},
        }),
        code: 200,
        expect: {
            result: {
                finished: false,
                expires: '2020-01-01T11:59:59.000Z',
            },
        },
        wrapResult: false,
    });

    responds({
        t,
        res: await callInternalQuasarApi('get', '/sub_state', {
            query: { id: uniqueString() },
        }),
        code: 200,
        expect: {
            result: {
                finished: false,
                expires: '2020-01-01T11:59:59.000Z',
            },
        },
        wrapResult: false,
    });

    responds({
        t,
        res: await callInternalQuasarApi('get', '/sub_state', {
            query: { platform: Platform.YandexStation },
        }),
        code: 200,
        expect: {
            result: {
                finished: false,
                expires: '2020-01-01T11:59:59.000Z',
            },
        },
        wrapResult: false,
    });
});

test('Неизвестное устройство', async (t) => {
    const now = new Date('2020-01-01T12:00:00Z').valueOf();
    sinon.stub(Date, 'now').returns(now);

    const res = await callInternalQuasarApi('get', '/sub_state', {
        query: { id: uniqueString(), platform: Platform.YandexStation },
    });

    responds({
        t,
        res,
        code: 200,
        expect: {
            result: {
                finished: false,
                expires: '2020-01-01T11:59:59.000Z',
            },
        },
        wrapResult: false,
    });
});

test('Устройство в статусе Reset', async (t) => {
    t.context.device = await createDevice({
        organizationId: t.context.organization.id,
        status: Status.Reset,
    });

    const now = new Date('2020-01-01T12:00:00Z').valueOf();
    sinon.stub(Date, 'now').returns(now);

    const res = await callInternalQuasarApi('get', '/sub_state', {
        query: { id: t.context.device.deviceId, platform: t.context.device.platform },
    });

    responds({
        t,
        res,
        code: 200,
        expect: {
            result: {
                finished: false,
                expires: '2020-01-01T11:59:59.000Z',
            },
        },
        wrapResult: false,
    });
});

test('Устройство в статусе Inactive', async (t) => {
    t.context.device = await createDevice({
        organizationId: t.context.organization.id,
        status: Status.Inactive,
    });

    const now = new Date('2020-01-01T12:00:00Z').valueOf();
    sinon.stub(Date, 'now').returns(now);

    const res = await callInternalQuasarApi('get', '/sub_state', {
        query: { id: t.context.device.deviceId, platform: t.context.device.platform },
    });

    responds({
        t,
        res,
        code: 200,
        expect: {
            result: {
                finished: false,
                expires: '2020-01-01T11:59:59.000Z',
            },
        },
        wrapResult: false,
    });
});
test('Устройство в статусе Active', async (t) => {
    t.context.device = await createDevice({
        organizationId: t.context.organization.id,
        status: Status.Active,
    });

    const now = new Date('2020-01-01T12:00:00Z').valueOf();
    sinon.stub(Date, 'now').returns(now);

    const res = await callInternalQuasarApi('get', '/sub_state', {
        query: { id: t.context.device.deviceId, platform: t.context.device.platform },
    });

    responds({
        t,
        res,
        code: 200,
        expect: {
            result: {
                finished: false,
                expires: '2020-01-01T12:30:00.000Z',
            },
        },
        wrapResult: false,
    });
});

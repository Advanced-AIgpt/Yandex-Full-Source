import anyTest, { TestInterface } from 'ava';
import sinon from 'sinon';
import { createDevice, wipeDatabase } from '../../helpers/db';
import { callPublicApi, createCredentials, responds } from '../../helpers/api';
import * as QuasarSync from '../../../lib/sync/quasar';
import { Status } from '../../../db/tables/device';
import { syncDeviceStub } from '../../helpers/status-sync';
import * as blackbox from '../../../services/blackbox';

interface Context {
    blackBoxStub: sinon.SinonStub;
}

const test = anyTest as TestInterface<Context>;

test.beforeEach(async (t) => {
    await wipeDatabase();
    await createCredentials();
    t.context.blackBoxStub = sinon
        .stub(blackbox, 'getUserHasPlus');
});
test.afterEach.always(async (t) => {
    sinon.restore();
});

test('Отдает информацию по device_id; online', async (t) => {
    const { externalDeviceId, deviceId, note } = await createDevice({
        status: Status.Inactive,
        online: false,
    });
    sinon.stub(QuasarSync, 'syncDevice').value(
        syncDeviceStub({
            status: Status.Active,
            online: true,
        }),
    );

    const res = await callPublicApi('get', '/devices/info', {
        query: { device_id: deviceId },
    });
    responds({
        expect: {
            external_id: externalDeviceId,
            status: 'ok',
            info: {
                external_id: externalDeviceId,
                device_id: deviceId,
                has_custom_user: false,
                online: true,
                status: 'active',
                in_progress: false,
                note,
                has_plus: null,
            },
        },
        wrapResult: false,
        code: 200,
        res,
        t,
    });
});

test('Отдает информацию по external_id; online', async (t) => {
    const { externalDeviceId, deviceId, note } = await createDevice({
        status: Status.Inactive,
        online: false,
    });
    sinon.stub(QuasarSync, 'syncDevice').value(
        syncDeviceStub({
            status: Status.Active,
            online: true,
        }),
    );

    const res = await callPublicApi('get', '/devices/info', {
        query: { external_id: externalDeviceId },
    });
    responds({
        expect: {
            external_id: externalDeviceId,
            status: 'ok',
            info: {
                external_id: externalDeviceId,
                device_id: deviceId,
                has_custom_user: false,
                online: true,
                status: 'active',
                note,
                in_progress: false,
                has_plus: null,
            },
        },
        wrapResult: false,
        code: 200,
        res,
        t,
    });
});

test('Отдает информацию по device_id; offline', async (t) => {
    const { externalDeviceId, deviceId, note } = await createDevice({
        status: Status.Inactive,
        online: true,
    });
    sinon.stub(QuasarSync, 'syncDevice').value(
        syncDeviceStub({
            status: Status.Active,
            online: false,
        }),
    );

    const res = await callPublicApi('get', '/devices/info', {
        query: { device_id: deviceId },
    });
    responds({
        expect: {
            external_id: externalDeviceId,
            status: 'ok',
            info: {
                external_id: externalDeviceId,
                device_id: deviceId,
                has_custom_user: false,
                online: false,
                status: 'active',
                in_progress: false,
                note,
                has_plus: null,
            },
        },
        wrapResult: false,
        code: 200,
        res,
        t,
    });
});

test('Отдает информацию по external_id; offline', async (t) => {
    const { externalDeviceId, deviceId, note, status } = await createDevice({
        status: Status.Inactive,
        online: true,
    });
    sinon.stub(QuasarSync, 'syncDevice').value(
        syncDeviceStub({
            status: Status.Active,
            online: false,
        }),
    );

    const res = await callPublicApi('get', '/devices/info', {
        query: { external_id: externalDeviceId },
    });
    responds({
        expect: {
            external_id: externalDeviceId,
            status: 'ok',
            info: {
                external_id: externalDeviceId,
                device_id: deviceId,
                has_custom_user: false,
                online: false,
                status: 'active',
                in_progress: false,
                note,
                has_plus: null,
            },
        },
        wrapResult: false,
        code: 200,
        res,
        t,
    });
});

test('Отдает информацию по device_id; owner user has plus', async (t) => {
    const { externalDeviceId, deviceId, note } = await createDevice({
        status: Status.Inactive,
        online: false,
        ownerId: '1111111111',
    });
    sinon.stub(QuasarSync, 'syncDevice').value(
        syncDeviceStub({
            status: Status.Active,
            online: true,
            ownerId: '1111111111',
        }),
    );
    t.context.blackBoxStub.withArgs('1111111111', sinon.match.any).resolves(true);

    const res = await callPublicApi('get', '/devices/info', {
        query: { device_id: deviceId },
    });
    responds({
        expect: {
            external_id: externalDeviceId,
            status: 'ok',
            info: {
                external_id: externalDeviceId,
                device_id: deviceId,
                has_custom_user: true,
                online: true,
                status: 'active',
                in_progress: false,
                note,
                has_plus: true,
            },
        },
        wrapResult: false,
        code: 200,
        res,
        t,
    });
});

test('Отдает информацию по device_id; owner user hasn`t plus', async (t) => {
    const { externalDeviceId, deviceId, note } = await createDevice({
        status: Status.Inactive,
        online: false,
        ownerId: '1111111111',
    });
    sinon.stub(QuasarSync, 'syncDevice').value(
        syncDeviceStub({
            status: Status.Active,
            online: true,
            ownerId: '1111111111',
        }),
    );
    t.context.blackBoxStub.withArgs('1111111111', sinon.match.any).resolves(false);

    const res = await callPublicApi('get', '/devices/info', {
        query: { device_id: deviceId },
    });
    responds({
        expect: {
            external_id: externalDeviceId,
            status: 'ok',
            info: {
                external_id: externalDeviceId,
                device_id: deviceId,
                has_custom_user: true,
                online: true,
                status: 'active',
                in_progress: false,
                note,
                has_plus: false,
            },
        },
        wrapResult: false,
        code: 200,
        res,
        t,
    });
});

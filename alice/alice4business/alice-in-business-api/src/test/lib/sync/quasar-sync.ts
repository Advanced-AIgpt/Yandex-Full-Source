import test from 'ava';
import sinon from 'sinon';
import uniqueString from 'unique-string';
import { createDevice, createOrganization, wipeDatabase } from '../../helpers/db';
import * as sendService from '../../../services/push/send';
import * as quasarService from '../../../services/quasar/info';
import data from '../../helpers/data';
import { Status } from '../../../db/tables/device';
import { syncDevice } from '../../../lib/sync/quasar';

test.beforeEach(async (t) => {
    await wipeDatabase();
    await createOrganization();
});

test.afterEach.always(async (t) => {
    sinon.restore();
});

test('Quasar вернул offline', async (t) => {
    // prepare
    const sendPush = sinon.stub(sendService, 'sendPush').returns();

    const device = await createDevice({
        status: Status.Active,
        organizationId: data.organization.id,
        online: true,
        ownerId: data.kolonkish.uid,
    });

    const ownerId = uniqueString();
    sinon.stub(quasarService, 'getDeviceInfo').resolves(
        data.response.getDeviceInfo({
            device: { status: 'offline', owner_uid: ownerId },
        }),
    );

    // test
    await syncDevice(device);
    t.is(device.status, Status.Active);
    t.false(device.online);
    t.is(device.ownerId, ownerId);

    t.is(sendPush.callCount, 1);
});

test('Quasar вернул online', async (t) => {
    // prepare
    const sendPush = sinon.stub(sendService, 'sendPush').returns();

    const device = await createDevice({
        status: Status.Active,
        organizationId: data.organization.id,
        online: false,
        ownerId: data.kolonkish.uid,
    });

    const ownerId = uniqueString();
    sinon.stub(quasarService, 'getDeviceInfo').resolves(
        data.response.getDeviceInfo({
            device: { status: 'online', owner_uid: ownerId },
        }),
    );

    // test
    await syncDevice(device);
    t.is(device.status, Status.Active);
    t.true(device.online);
    t.is(device.ownerId, ownerId);

    t.is(sendPush.callCount, 1);
});

test('Состояние устройства не поменялось', async (t) => {
    // prepare
    const sendPush = sinon.stub(sendService, 'sendPush').returns();

    const device = await createDevice({
        status: Status.Active,
        organizationId: data.organization.id,
        online: false,
        ownerId: data.kolonkish.uid,
    });

    sinon.stub(quasarService, 'getDeviceInfo').resolves(
        data.response.getDeviceInfo({
            device: { status: 'offline', owner_uid: data.kolonkish.uid },
        }),
    );

    // test
    await syncDevice(device);
    t.is(device.status, Status.Active);
    t.false(device.online);
    t.is(device.ownerId, data.kolonkish.uid);

    t.is(sendPush.callCount, 0);
});

test('Quasar error', async (t) => {
    // prepare
    const sendPush = sinon.stub(sendService, 'sendPush').returns();

    const device = await createDevice({
        status: Status.Active,
        organizationId: data.organization.id,
        online: true,
        ownerId: data.kolonkish.uid,
    });

    sinon.stub(quasarService, 'getDeviceInfo').rejects();

    // test
    await t.throwsAsync(syncDevice(device));
    t.is(device.status, Status.Active);
    t.true(device.online);
    t.is(device.ownerId, data.kolonkish.uid);

    t.is(sendPush.callCount, 0);
});

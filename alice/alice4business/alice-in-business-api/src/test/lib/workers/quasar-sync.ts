import test from 'ava';
import sinon from 'sinon';
import * as quasarService from '../../../services/quasar/info';
import { quasarSyncWorker } from '../../../lib/workers/quasar-sync';
import { bulkCreateDevices, createOrganization, wipeDatabase } from '../../helpers/db';
import { startWorkers, stopWorkers } from '../../helpers/workers';
import { sleep } from '../../../lib/utils';
import config from '../../../lib/config';
import { Device } from '../../../db';
import data from '../../helpers/data';
import * as sendService from '../../../services/push/send';

test.beforeEach(async () => {
    await wipeDatabase();
    await startWorkers(quasarSyncWorker, 10);
    await createOrganization();
});

test.afterEach.always(async () => {
    await stopWorkers(quasarSyncWorker);
    await sleep(config.app.quasarSyncWorker.interval * 2);
    sinon.restore();
});

test('Дожидается retryInterval', async (t) => {
    const sendPush = sinon.stub(sendService, 'sendPush').returns();
    sinon.stub(quasarService, 'getDeviceInfo').resolves(data.response.getDeviceInfo());

    const now = Date.now();
    const actualDevices = await bulkCreateDevices(10, {
        online: false,
        lastSyncStart: new Date(now),
    });
    const outdatedDevices = await bulkCreateDevices(10, {
        online: false,
        lastSyncStart: new Date(
            now -
                (config.quasar.retryInterval - config.app.quasarSyncWorker.interval * 3),
        ),
    });

    await sleep(config.app.quasarSyncWorker.interval * 2);
    t.is(await Device.count({ where: { online: true } }), 0);
    t.is(sendPush.callCount, 0);

    await sleep(config.app.quasarSyncWorker.interval * 2);
    t.is(await Device.count({ where: { online: true } }), outdatedDevices.length);
    t.is(sendPush.callCount, outdatedDevices.length);
});

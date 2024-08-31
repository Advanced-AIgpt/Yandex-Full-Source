import test from 'ava';
import sinon from 'sinon';
import { v4 as uuidv4 } from 'uuid';
import { startWorkers, stopWorkers } from '../../helpers/workers';
import * as worker from '../../../lib/workers/operation-killer';
import { createDevice, createOrganization, wipeDatabase } from '../../helpers/db';
import {
    OperationSchema,
    Status as OperationStatus,
    Type as OperationType,
} from '../../../db/tables/operation';
import { sleep } from '../../../lib/utils';
import config from '../../../lib/config';
import { Operation } from '../../../db';
import * as sendService from '../../../services/push/send';

test.beforeEach(async (t) => {
    await wipeDatabase();
    await createOrganization();
    await startWorkers(worker.operationKillerWorker, 10);
});

test.afterEach.always(async (t) => {
    await stopWorkers(worker.operationKillerWorker);
    await sleep(config.app.operationsWorker.interval * 2);
    sinon.restore();
});

test('Reject по таймауту: новая операция', async (t) => {
    const sendPush = sinon.stub(sendService, 'sendPush').returns();

    const device = await createDevice();
    const old = {
        status: device.status,
        kolonkishId: device.kolonkishId,
        kolonkishLogin: device.kolonkishLogin,
    };

    const operation = await Operation.create({
        id: uuidv4(),
        type: [OperationType.Reset, OperationType.Activate, OperationType.PromoActivate][
            Math.floor(Math.random() * 3)
        ],
        devicePk: device.id,
        status: OperationStatus.Pending,
        lastHandling: new Date(),
    } as OperationSchema);

    await sleep(config.app.operationsWorker.interval * 2);
    await device.reload();
    await operation.reload();

    t.is(device.status, old.status);
    t.is(device.kolonkishId, old.kolonkishId);
    t.is(device.kolonkishLogin, old.kolonkishLogin);

    t.is(operation.status, OperationStatus.Pending);
    t.falsy(operation.error);

    t.is(sendPush.callCount, 0);
});

test('Reject по таймауту: протухающая операция', async (t) => {
    const sendPush = sinon.stub(sendService, 'sendPush').returns();

    const device = await createDevice();
    const old = {
        status: device.status,
        kolonkishId: device.kolonkishId,
        kolonkishLogin: device.kolonkishLogin,
    };

    const operation = await Operation.create({
        type: [OperationType.Reset, OperationType.Activate, OperationType.PromoActivate][
            Math.floor(Math.random() * 3)
        ],
        devicePk: device.id,
        status: OperationStatus.Pending,
        lastHandling: new Date(
            Date.now() -
                (config.app.operationsWorker.rejectTimeout -
                    config.app.operationsWorker.interval * 3),
        ),
    } as OperationSchema);

    await sleep(config.app.operationsWorker.interval * 2);
    await device.reload();
    await operation.reload();

    t.is(device.status, old.status);
    t.is(device.kolonkishId, old.kolonkishId);
    t.is(device.kolonkishLogin, old.kolonkishLogin);

    t.is(operation.status, OperationStatus.Pending);
    t.falsy(operation.error);

    t.is(sendPush.callCount, 0);
});

test('Reject по таймауту: операция протухла', async (t) => {
    const sendPush = sinon.stub(sendService, 'sendPush').returns();

    const device = await createDevice();
    const old = {
        status: device.status,
        kolonkishId: device.kolonkishId,
        kolonkishLogin: device.kolonkishLogin,
    };

    const operation = await Operation.create({
        type: [OperationType.Reset, OperationType.Activate, OperationType.PromoActivate][
            Math.floor(Math.random() * 3)
        ],
        devicePk: device.id,
        status: OperationStatus.Pending,
        lastHandling: new Date(Date.now() - config.app.operationsWorker.rejectTimeout),
    } as OperationSchema);

    await sleep(config.app.operationsWorker.interval * 2);
    await device.reload();
    await operation.reload();

    t.is(device.status, old.status);
    t.is(device.kolonkishId, old.kolonkishId);
    t.is(device.kolonkishLogin, old.kolonkishLogin);

    t.is(operation.status, OperationStatus.Rejected);
    t.truthy(operation.error);
    t.is(operation.error.message, 'Timeout');

    t.is(sendPush.callCount, 1);
});

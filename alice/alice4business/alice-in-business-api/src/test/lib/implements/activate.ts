import test from 'ava';
import sinon from 'sinon';
import {
    OperationSchema,
    Status as OperationStatus,
    Type as OperationType,
} from '../../../db/tables/operation';
import { activateImplement } from '../../../lib/implements/activate';
import * as Send from '../../../services/push/send';
import { createCredentials } from '../../helpers/api';
import data from '../../helpers/data';
import { createDevice, getLastOperation, wipeDatabase } from '../../helpers/db';
import { Device, Operation } from '../../../db';
import RestApiError from '../../../lib/errors/restApi';
import { Status } from '../../../db/tables/device';

test.beforeEach(async (t) => {
    await wipeDatabase();
    await createCredentials();
});

test.afterEach.always(async (t) => {
    sinon.restore();
});

test('Ошибка при создании операции: устройство занято другой операцией', async (t) => {
    const sendPush = sinon.stub(Send, 'sendPush').returns();

    const device = await createDevice();
    const old = {
        status: device.status,
        kolonkishId: device.kolonkishId,
        kolonkishLogin: device.kolonkishLogin,
    };

    await Operation.create({
        devicePk: device.id,
        type: OperationType.PromoActivate,
        status: OperationStatus.Pending,
        lastHandling: new Date(),
    } as OperationSchema);

    const error: RestApiError = await t.throwsAsync(
        activateImplement(device, data.operation.scope!),
        {
            instanceOf: RestApiError,
            message: 'Device is busy',
        },
    );
    t.is(error.statusCode, 409);

    const op = await getLastOperation(device.id);
    t.truthy(op);
    t.is(op!.status, OperationStatus.Rejected);
    t.truthy(op!.error);

    await device.reload();
    t.is(device.status, old.status);
    t.is(device.kolonkishId, old.kolonkishId);
    t.is(device.kolonkishLogin, old.kolonkishLogin);

    t.is(sendPush.callCount, 0);
});

test('Ошибка при создании операции: что-то пошло не так', async (t) => {
    const sendPush = sinon.stub(Send, 'sendPush').returns();

    const device = await createDevice();
    const old = {
        status: device.status,
        kolonkishId: device.kolonkishId,
        kolonkishLogin: device.kolonkishLogin,
    };

    sinon.stub(Operation, 'create').callThrough().onFirstCall().rejects();

    const error: RestApiError = await t.throwsAsync(
        activateImplement(device, data.operation.scope!),
        {
            instanceOf: RestApiError,
            message: 'Unexpected error',
        },
    );
    t.is(error.statusCode, 500);

    const op = await getLastOperation(device.id);
    t.truthy(op);
    t.is(op!.status, OperationStatus.Rejected);
    t.truthy(op!.error);

    await device.reload();
    t.is(device.status, old.status);
    t.is(device.kolonkishId, old.kolonkishId);
    t.is(device.kolonkishLogin, old.kolonkishLogin);

    t.is(sendPush.callCount, 0);
});

test('Ошибка: устройство уже активно', async (t) => {
    const sendPush = sinon.stub(Send, 'sendPush').returns();

    const device = await createDevice({
        status: Status.Active,
    });
    const old = {
        status: device.status,
        kolonkishId: device.kolonkishId,
        kolonkishLogin: device.kolonkishLogin,
    };

    const error: RestApiError = await t.throwsAsync(
        activateImplement(device, data.operation.scope!),
        {
            instanceOf: RestApiError,
            message: 'Device status is not Inactive',
        },
    );
    t.is(error.statusCode, 409);

    const op = await getLastOperation(device.id);
    t.truthy(op);
    t.is(op!.status, OperationStatus.Rejected);
    t.truthy(op!.error);

    await device.reload();
    t.is(device.status, old.status);
    t.is(device.kolonkishId, old.kolonkishId);
    t.is(device.kolonkishLogin, old.kolonkishLogin);

    t.is(sendPush.callCount, 2);
});

test('Ошибка: сброс не был завершён', async (t) => {
    const sendPush = sinon.stub(Send, 'sendPush').returns();

    const device = await createDevice({
        status: Status.Inactive,
        kolonkishId: undefined,
        kolonkishLogin: undefined,
    });
    const old = {
        status: device.status,
        kolonkishId: device.kolonkishId,
        kolonkishLogin: device.kolonkishLogin,
    };

    const error: RestApiError = await t.throwsAsync(
        activateImplement(device, data.operation.scope!),
        {
            instanceOf: RestApiError,
            message: 'Device status is not Inactive',
        },
    );
    t.is(error.statusCode, 409);

    const op = await getLastOperation(device.id);
    t.truthy(op);
    t.is(op!.status, OperationStatus.Rejected);
    t.truthy(op!.error);

    await device.reload();
    t.is(device.status, old.status);
    t.is(device.kolonkishId, old.kolonkishId);
    t.is(device.kolonkishLogin, old.kolonkishLogin);

    t.is(sendPush.callCount, 2);
});

test('Ошибка: операция была отменена', async (t) => {
    const sendPush = sinon.stub(Send, 'sendPush').returns();

    const device = await createDevice({
        status: Status.Inactive,
    });
    const old = {
        status: device.status,
        kolonkishId: device.kolonkishId,
        kolonkishLogin: device.kolonkishLogin,
    };

    sinon.stub(Operation, 'update').callThrough().onFirstCall().resolves([0, []]);

    const error: RestApiError = await t.throwsAsync(
        activateImplement(device, data.operation.scope!),
        {
            instanceOf: RestApiError,
            message: 'Operation was cancelled',
        },
    );
    t.is(error.statusCode, 409);

    const op = await getLastOperation(device.id);
    t.truthy(op);
    t.is(op!.status, OperationStatus.Rejected);
    t.truthy(op!.error);

    await device.reload();
    t.is(device.status, old.status);
    t.is(device.kolonkishId, old.kolonkishId);
    t.is(device.kolonkishLogin, old.kolonkishLogin);

    t.is(sendPush.callCount, 2);
});

test('Ошибка: база сломалась после создания операции', async (t) => {
    const sendPush = sinon.stub(Send, 'sendPush').returns();

    const device = await createDevice({});
    const old = {
        status: device.status,
        kolonkishId: device.kolonkishId,
        kolonkishLogin: device.kolonkishLogin,
    };

    sinon.stub(Operation, 'update').rejects();
    sinon.stub(Device, 'update').rejects();

    const error: RestApiError = await t.throwsAsync(
        activateImplement(device, data.operation.scope!),
        {
            instanceOf: RestApiError,
            message: 'Unexpected error',
        },
    );
    t.is(error.statusCode, 500);

    const op = await getLastOperation(device.id);
    t.truthy(op);
    t.is(op!.status, OperationStatus.Pending);
    t.falsy(op!.error);

    await device.reload();
    t.is(device.status, old.status);
    t.is(device.kolonkishId, old.kolonkishId);
    t.is(device.kolonkishLogin, old.kolonkishLogin);

    t.is(sendPush.callCount, 2);
});

test('Pipeline успешно выполнен, возвращаем operationId', async (t) => {
    const sendPush = sinon.stub(Send, 'sendPush').returns();

    const device = await createDevice({
        status: Status.Inactive,
    });
    const old = {
        kolonkishId: device.kolonkishId,
        kolonkishLogin: device.kolonkishLogin,
    };

    const expectOpId = await activateImplement(device, data.operation.scope!);

    const op = await getLastOperation(device.id);
    t.truthy(op);
    t.is(op!.id, expectOpId);
    t.is(op!.status, OperationStatus.Resolved);
    t.falsy(op!.error);

    await device.reload();
    t.is(device.status, Status.Active);
    t.is(device.kolonkishId, old.kolonkishId);
    t.is(device.kolonkishLogin, old.kolonkishLogin);

    t.is(sendPush.callCount, 2);
});

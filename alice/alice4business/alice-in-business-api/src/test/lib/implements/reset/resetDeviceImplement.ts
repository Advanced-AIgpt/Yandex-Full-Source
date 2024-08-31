import anyTest, { TestInterface } from 'ava';
import sinon from 'sinon';
import uniqueString from 'unique-string';
import { v4 as uuidv4 } from 'uuid';
import RestApiError from '../../../../lib/errors/restApi';
import { DeviceInstance, DeviceSchema, Status } from '../../../../db/tables/device';
import {
    OperationSchema,
    Status as OperationStatus,
    Type as OperationType,
} from '../../../../db/tables/operation';
import data from '../../../helpers/data';
import { createDevice, getLastOperation } from '../../../helpers/db';
import { Device, Operation, Organization } from '../../../../db';
import * as Reset from '../../../../lib/implements/reset';
import * as Kolonkish from '../../../../services/passport/kolonkish';
import * as Send from '../../../../services/push/send';
import * as Switch from '../../../../services/quasar/switch';
import {
    OrganizationInstance,
    OrganizationSchema,
} from '../../../../db/tables/organization';
import { sleep } from '../../../../lib/utils';

interface Context {
    organization: OrganizationInstance;
    device: DeviceInstance;

    _switchUser: sinon.SinonStub;
    _sendPush: sinon.SinonStub;
}
const test = anyTest as TestInterface<Context>;

test.beforeEach(async (t) => {
    t.context.organization = await Organization.create({
        id: uuidv4(),
        name: `${t.title} ${uniqueString()}`,
    } as OrganizationSchema);
    t.context.device = await createDevice({
        organizationId: t.context.organization.id,
        status: Status.Active,
        agreementAccepted: true,
    });

    t.context._switchUser = sinon.stub(Switch, 'switchDeviceUser').resolves();
    t.context._sendPush = sinon.stub(Send, 'sendPush').returns();
});

test.afterEach.always(async (t) => {
    sinon.restore();

    await t.context.device.destroy();
    await t.context.organization.destroy();
});

test('Ошибка при создании операции: обновление устройства', async (t) => {
    const device = t.context.device;
    const old = {
        status: device.status,
        kolonkishId: device.kolonkishId,
        kolonkishLogin: device.kolonkishLogin,
    };
    sinon.stub(Device, 'update').callThrough().onFirstCall().rejects();
    const error: RestApiError = await t.throwsAsync(
        Reset.resetDeviceImplement(
            device,
            data.user.ip,
            data.headers,
            data.operation.scope!,
        ),
        {
            instanceOf: RestApiError,
            message: 'Unexpected error',
        },
    );
    t.is(error.statusCode, 500);

    const op = await getLastOperation(device.id);
    t.truthy(op);
    t.is(op!.type, OperationType.Reset);
    t.is(op!.status, OperationStatus.Rejected);
    t.truthy(op!.error);

    await device.reload();
    t.is(device.status, old.status);
    t.is(device.kolonkishId, old.kolonkishId);
    t.is(device.kolonkishLogin, old.kolonkishLogin);

    t.is(t.context._switchUser.callCount, 0);
    t.is(t.context._sendPush.callCount, 0);
});

test('Ошибка при создании операции: создание операции', async (t) => {
    const device = t.context.device;
    const old = {
        status: device.status,
        kolonkishId: device.kolonkishId,
        kolonkishLogin: device.kolonkishLogin,
    };
    sinon.stub(Operation, 'create').callThrough().onFirstCall().rejects();

    const error: RestApiError = await t.throwsAsync(
        Reset.resetDeviceImplement(
            device,
            data.user.ip,
            data.headers,
            data.operation.scope!,
        ),
        {
            instanceOf: RestApiError,
            message: 'Unexpected error',
        },
    );
    t.is(error.statusCode, 500);

    const op = await getLastOperation(device.id);
    t.truthy(op);
    t.is(op!.type, OperationType.Reset);
    t.is(op!.status, OperationStatus.Rejected);
    t.truthy(op!.error);

    await device.reload();
    t.is(device.status, old.status);
    t.is(device.kolonkishId, old.kolonkishId);
    t.is(device.kolonkishLogin, old.kolonkishLogin);

    t.is(t.context._switchUser.callCount, 0);
    t.is(t.context._sendPush.callCount, 0);
});

test('Ошибка при создании операции: устройство занято другой операцией', async (t) => {
    const device = t.context.device;
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
        Reset.resetDeviceImplement(
            device,
            data.user.ip,
            data.headers,
            data.operation.scope!,
        ),
        {
            instanceOf: RestApiError,
            message: 'Device is busy',
        },
    );
    t.is(error.statusCode, 409);

    const op = await getLastOperation(device.id);
    t.truthy(op);
    t.is(op!.type, OperationType.Reset);
    t.is(op!.status, OperationStatus.Rejected);
    t.truthy(op!.error);

    await device.reload();
    t.is(device.status, old.status);
    t.is(device.kolonkishId, old.kolonkishId);
    t.is(device.kolonkishLogin, old.kolonkishLogin);

    t.is(t.context._switchUser.callCount, 0);
    t.is(t.context._sendPush.callCount, 0);
});

test('Ошибка при создании операции: устройство занято операцией сброса', async (t) => {
    const device = t.context.device;
    await device.update({
        status: Status.Reset,
        kolonkishId: null,
        kolonkishLogin: null,
    } as DeviceSchema);
    await device.reload(); // костыль, чтобы получить updatedAt
    const operation = await Operation.create({
        devicePk: device.id,
        type: OperationType.Reset,
        status: OperationStatus.Pending,
        lastHandling: new Date(),
    } as OperationSchema);
    await operation.reload(); // костыль, чтобы получить updatedAt
    const initialUpdatedAt = operation.updatedAt;

    const operationId = await Reset.resetDeviceImplement(
        device,
        data.user.ip,
        data.headers,
        data.operation.scope!,
    );

    t.is(operationId, operation.id);

    await operation.reload();
    t.is(operation.status, OperationStatus.Pending);
    t.is(operation.updatedAt.valueOf(), initialUpdatedAt.valueOf());

    await device.reload();
    t.is(device.status, Status.Reset);
    t.is(device.kolonkishId, null);
    t.is(device.kolonkishLogin, null);

    t.is(t.context._switchUser.callCount, 0);
    t.is(t.context._sendPush.callCount, 0);
});

test('Ошибка регистрации колонкиша', async (t) => {
    const device = t.context.device;
    sinon.stub(Kolonkish, 'registryKolonkish').rejects();

    const error: RestApiError = await t.throwsAsync(
        Reset.resetDeviceImplement(
            device,
            data.user.ip,
            data.headers,
            data.operation.scope!,
        ),
        {
            instanceOf: RestApiError,
            message: 'Failed to register account',
        },
    );
    t.is(error.statusCode, 500);

    const op = await getLastOperation(device.id);
    t.truthy(op);
    t.is(op!.type, OperationType.Reset);
    t.is(op!.status, OperationStatus.Rejected);
    t.truthy(op!.error);

    await device.reload();
    t.is(device.status, Status.Reset);
    t.is(device.kolonkishId, null);
    t.is(device.kolonkishLogin, null);

    t.is(t.context._switchUser.callCount, 0);
    t.is(t.context._sendPush.callCount, 2);
});

test('Ошибка записи колонкиша в операцию', async (t) => {
    const device = t.context.device;
    const kolonkish: Kolonkish.KolonkishResponse = {
        login: uniqueString(),
        uid: uniqueString(),
        status: 'ok',
        code: uniqueString(),
    };
    sinon.stub(Kolonkish, 'registryKolonkish').resolves(kolonkish);
    sinon.stub(Operation, 'update').callThrough().onFirstCall().rejects();

    const error: RestApiError = await t.throwsAsync(
        Reset.resetDeviceImplement(
            device,
            data.user.ip,
            data.headers,
            data.operation.scope!,
        ),
        {
            instanceOf: RestApiError,
            message: 'Failed to update operation',
        },
    );
    t.is(error.statusCode, 500);

    const op = await getLastOperation(device.id);
    t.truthy(op);
    t.is(op!.type, OperationType.Reset);
    t.is(op!.status, OperationStatus.Rejected);
    t.truthy(op!.error);

    await device.reload();
    t.is(device.status, Status.Reset);
    t.is(device.kolonkishId, null);
    t.is(device.kolonkishLogin, null);

    t.is(t.context._switchUser.callCount, 0);
    t.is(t.context._sendPush.callCount, 2);
});

test('Ошибка записи колонкиша в операцию: операция была отменена извне', async (t) => {
    const device = t.context.device;
    const kolonkish: Kolonkish.KolonkishResponse = {
        login: uniqueString(),
        uid: uniqueString(),
        status: 'ok',
        code: uniqueString(),
    };
    sinon.stub(Kolonkish, 'registryKolonkish').resolves(kolonkish);
    sinon.stub(Operation, 'update').callThrough().onFirstCall().resolves([0, []]);

    const error: RestApiError = await t.throwsAsync(
        Reset.resetDeviceImplement(
            device,
            data.user.ip,
            data.headers,
            data.operation.scope!,
        ),
        {
            instanceOf: RestApiError,
            message: 'Failed to update operation',
        },
    );
    t.is(error.statusCode, 500);

    const op = await getLastOperation(device.id);
    t.truthy(op);
    t.is(op!.type, OperationType.Reset);
    t.is(op!.status, OperationStatus.Rejected);
    t.truthy(op!.error);

    await device.reload();
    t.is(device.status, Status.Reset);
    t.is(device.kolonkishId, null);
    t.is(device.kolonkishLogin, null);

    t.is(t.context._switchUser.callCount, 0);
    t.is(t.context._sendPush.callCount, 2);
});

test('Успех, возвращается id операции', async (t) => {
    const device = t.context.device;
    const kolonkish: Kolonkish.KolonkishResponse = {
        login: uniqueString(),
        uid: uniqueString(),
        status: 'ok',
        code: uniqueString(),
    };
    sinon.stub(Kolonkish, 'registryKolonkish').resolves(kolonkish);
    t.context._switchUser.resolves();

    const operationId = await Reset.resetDeviceImplement(
        device,
        data.user.ip,
        data.headers,
        data.operation.scope!,
    );
    const op = await Operation.findByPk(operationId);
    t.truthy(op);
    t.is(op!.type, OperationType.Reset);
    t.is(op!.status, OperationStatus.Pending);
    t.truthy(op!.payload?.kolonkishUid);
    t.truthy(op!.payload?.kolonkishLogin);

    await device.reload();
    t.is(device.status, Status.Reset);
    t.is(device.kolonkishId, null);
    t.is(device.kolonkishLogin, null);
    t.is(device.agreementAccepted, false);
});

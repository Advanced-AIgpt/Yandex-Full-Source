import anyTest, { TestInterface } from 'ava';
import sinon from 'sinon';
import { v4 as uuidv4 } from 'uuid';
import uniqueString from 'unique-string';

import {
    OperationSchema,
    Status as OperationStatus,
    Type as OperationType,
    UserScope,
} from '../../../db/tables/operation';
import data from '../../helpers/data';
import * as Send from '../../../services/push/send';
import { OrganizationInstance } from '../../../db/tables/organization';
import { DeviceInstance, DeviceSchema, Status } from '../../../db/tables/device';
import { createDevice, createOrganization, getLastOperation } from '../../helpers/db';
import RestApiError from '../../../lib/errors/restApi';
import { Device, Operation } from '../../../db';
import { createOperation } from '../../../lib/implements/operations';


interface Context {
    organization: OrganizationInstance;
    device: DeviceInstance;
}
const test = anyTest as TestInterface<Context>;

test.beforeEach(async (t) => {
    t.context.organization = await createOrganization({
        id: uuidv4(),
        name: `${t.title} ${uniqueString()}`,
    });
    t.context.device = await createDevice({
        organizationId: t.context.organization.id,
        status: Status.Active,
        agreementAccepted: true,
    });
});

test.afterEach.always(async (t) => {
    sinon.restore();

    await t.context.device.destroy({ force: true });
    await t.context.organization.destroy({ force: true });
});

test('Ошибка при сбросе устройства', async (t) => {
    const device = t.context.device;
    const old = {
        status: device.status,
        kolonkishId: device.kolonkishId,
        kolonkishLogin: device.kolonkishLogin,
        agreementAccepted: device.agreementAccepted,
    };
    sinon.stub(Device, 'update').callThrough().onFirstCall().rejects();

    const error: RestApiError = await t.throwsAsync(
        createOperation(
            device, 
            data.operation.scope!,
            OperationType.Reset,
            {
                status: Status.Reset,
                kolonkishId: null,
                kolonkishLogin: null,
                agreementAccepted: false,
            } as DeviceSchema,
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
    t.is(device.agreementAccepted, old.agreementAccepted);
});

test('Ошибка при создании операции', async (t) => {
    const device = t.context.device;
    const old = {
        status: device.status,
        kolonkishId: device.kolonkishId,
        kolonkishLogin: device.kolonkishLogin,
        agreementAccepted: device.agreementAccepted,
    };
    sinon.stub(Operation, 'create').callThrough().onFirstCall().rejects();

    const error: RestApiError = await t.throwsAsync(
        createOperation(
            device, 
            data.operation.scope!,
            OperationType.Reset,
            {
                status: Status.Reset,
                kolonkishId: null,
                kolonkishLogin: null,
                agreementAccepted: false,
            } as DeviceSchema,
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
    t.is(device.agreementAccepted, old.agreementAccepted);
});

test('Ошибка: устройство занято другой операцией', async (t) => {
    const device = t.context.device;
    const old = {
        status: device.status,
        kolonkishId: device.kolonkishId,
        kolonkishLogin: device.kolonkishLogin,
        agreementAccepted: device.agreementAccepted,
    };
    await Operation.create({
        devicePk: device.id,
        type: OperationType.PromoActivate,
        status: OperationStatus.Pending,
        lastHandling: new Date(),
    } as OperationSchema);

    const error: RestApiError = await t.throwsAsync(
        createOperation(
            device, 
            data.operation.scope!,
            OperationType.Reset,
            {
                status: Status.Reset,
                kolonkishId: null,
                kolonkishLogin: null,
                agreementAccepted: false,
            } as DeviceSchema,
        ),
        {
            instanceOf: RestApiError,
            message: 'Device is busy',
        },
    );
    t.is(error.statusCode, 409);

    const op = (await getLastOperation(device.id))!;
    t.is(op.type, OperationType.Reset);
    t.is(op.status, OperationStatus.Rejected);
    t.truthy(op.error);

    await device.reload();
    t.is(device.status, old.status);
    t.is(device.kolonkishId, old.kolonkishId);
    t.is(device.kolonkishLogin, old.kolonkishLogin);
    t.is(device.agreementAccepted, old.agreementAccepted);
});

test('Успешный сценарий', async (t) => {
    const device = t.context.device;
    t.not(device.status, Status.Reset);
    t.not(device.kolonkishId, undefined);
    t.not(device.kolonkishLogin, undefined);
    t.true(device.agreementAccepted);

    const operation = await createOperation(
        device, 
        data.operation.scope!,
        OperationType.Reset,
        {
            status: Status.Reset,
            kolonkishId: null,
            kolonkishLogin: null,
            agreementAccepted: false,
        } as DeviceSchema,
    );
    try {
        const { id } = (await getLastOperation(device.id))!;
        t.is(operation.id, id);
        t.is(operation.type, OperationType.Reset);
        t.is(operation.status, OperationStatus.Pending);
        t.falsy(operation.error);

        await device.reload();
        t.is(device.status, Status.Reset);
        t.is(device.kolonkishId, null);
        t.is(device.kolonkishLogin, null);
        t.false(device.agreementAccepted);
    } finally {
        await operation.destroy({ force: true });
    }
});
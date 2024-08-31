import anyTest, { TestInterface } from 'ava';
import sinon from 'sinon';
import { v4 as uuidv4 } from 'uuid';
import uniqueString from 'unique-string';
import * as Activate4Customer from '../../../../lib/implements/activate4customer';
import * as Send from '../../../../services/push/send';
import * as Passport from '../../../../services/passport/code';
import data from '../../../helpers/data';
import { createDevice, createOrganization, getLastOperation } from '../../../helpers/db';
import RestApiError from '../../../../lib/errors/restApi';
import { Operation } from '../../../../db';
import { DeviceInstance, Status } from '../../../../db/tables/device';
import {
    OperationInstance,
    OperationSchema,
    Status as OperationStatus,
    Type as OperationType,
} from '../../../../db/tables/operation';
import { OrganizationInstance } from '../../../../db/tables/organization';

interface Context {
    organization: OrganizationInstance;
    device: DeviceInstance;
    operation: OperationInstance;

    _getCodeForAm: sinon.SinonStub;
    _sendPush: sinon.SinonStub;
}
const test = anyTest as TestInterface<Context>;

test.beforeEach(async (t) => {
    t.context.organization = await createOrganization({
        id: uuidv4(),
        name: `${t.title} ${uniqueString()}`,
    });
    t.context.device = await createDevice({
        organizationId: t.context.organization.id,
        status: Status.Reset,
        agreementAccepted: false,
        kolonkishLogin: undefined,
        kolonkishId: undefined,
    });
    t.context.operation = await Operation.create({
        devicePk: t.context.device.id,
        type: OperationType.Activate,
        status: OperationStatus.Pending,
        lastHandling: new Date(),
        scope: {
            ...data.operation.scope,
            userLogin: await data.user.login,
            userId: data.user.uid.toString(10),
        },
    } as OperationSchema);

    t.context._getCodeForAm = sinon.stub(Passport, 'getCodeForAm');
    t.context._sendPush = sinon.stub(Send, 'sendPush').returns();
});

test.afterEach.always(async (t) => {
    sinon.restore();

    await t.context.operation.destroy({ force: true });
    await t.context.device.destroy({ force: true });
    await t.context.organization.destroy({ force: true });
});

test('Ошибка при получении x-code', async (t) => {
    t.context._getCodeForAm.rejects(new Error('smth bad happen'));

    const error: RestApiError = await t.throwsAsync(
        Activate4Customer._getOwnerCode(
            t.context.operation,
            t.context.device,
            data.user,
            data.headers,
        ),
        {
            instanceOf: RestApiError,
            message: 'Failed to get auth code',
        },
    );
    t.is(error.statusCode, 500);

    const op = (await getLastOperation(t.context.device.id))!;
    t.is(op.id, t.context.operation.id);
    t.is(op.status, OperationStatus.Rejected);
    t.truthy(op.error);

    t.is(t.context._sendPush.callCount, 1);
});

test('Получен пустой x-code', async (t) => {
    t.context._getCodeForAm.resolves('');

    const error: RestApiError = await t.throwsAsync(
        Activate4Customer._getOwnerCode(
            t.context.operation,
            t.context.device,
            data.user,
            data.headers,
        ),
        {
            instanceOf: RestApiError,
            message: 'Failed to get auth code',
        },
    );
    t.is(error.statusCode, 500);

    const op = (await getLastOperation(t.context.device.id))!;
    t.is(op.id, t.context.operation.id);
    t.is(op.status, OperationStatus.Rejected);
    t.truthy(op.error);

    t.is(t.context._sendPush.callCount, 1);
});

test('Успешный сценарий', async (t) => {
    t.context._getCodeForAm.resolves('some code');

    const code = await Activate4Customer._getOwnerCode(
        t.context.operation,
        t.context.device,
        data.user,
        data.headers,
    );
    t.is(code, 'some code');

    const op = (await getLastOperation(t.context.device.id))!;
    t.is(op.id, t.context.operation.id);
    t.is(op.status, t.context.operation.status);
    t.falsy(op.error);

    t.is(t.context._sendPush.callCount, 0);
});

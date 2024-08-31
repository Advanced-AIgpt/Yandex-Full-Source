import anyTest, { TestInterface } from 'ava';
import sinon from 'sinon';
import uniqueString from 'unique-string';
import { v4 as uuidv4 } from 'uuid';
import data from '../../../helpers/data';
import * as Activate4Customer from '../../../../lib/implements/activate4customer';
import * as Switch from '../../../../services/quasar/switch';
import * as Operations from '../../../../lib/implements/operations';
import { Device, Operation } from '../../../../db';
import {
    OperationSchema,
    Status as OperationStatus,
    Type as OperationType,
} from '../../../../db/tables/operation';
import { sleep } from '../../../../lib/utils';
import { createDevice, createOrganization } from '../../../helpers/db';
import { Status } from '../../../../db/tables/device';

interface Context {
    _getPendingOperation: sinon.SinonStub;
    _createMasterOperation: sinon.SinonStub;
    _getOwnerCode: sinon.SinonStub;
    _switchOwner: sinon.SinonStub;
    _rejectOperation: sinon.SinonStub;
    _updateOpTs: sinon.SinonStub;
}
const test = anyTest as TestInterface<Context>;

test.beforeEach(async (t) => {
    t.context._getPendingOperation = sinon.stub(Operation, 'findOne').resolves();
    t.context._createMasterOperation = sinon.stub(Operations, 'createOperation');
    t.context._getOwnerCode = sinon.stub(Activate4Customer, '_getOwnerCode');
    t.context._switchOwner = sinon.stub(Switch, 'switchDeviceUser');
    t.context._rejectOperation = sinon.stub(Operations, 'rejectOperation');
    t.context._updateOpTs = sinon.stub(Operations, 'updateOperationsLastHandling').resolves();
});
test.afterEach.always(async (t) => {
    sinon.restore();
});

test('returns pending operation id', async (t) => {
    t.context._getPendingOperation.restore();
    t.context._createMasterOperation.restore();

    const organization = await createOrganization({
        id: uuidv4(),
        name: `${t.title} ${uniqueString()}`,
    });
    try {
        const device = await createDevice({
            organizationId: organization.id,
            status: Status.Inactive,
            agreementAccepted: true,
        });
        try {
            const pendingOperation = await Operation.create({
                devicePk: device.id,
                type: OperationType.Activate,
                status: OperationStatus.Pending,
                lastHandling: new Date(),
                scope: {
                    ...data.operation.scope,
                    userLogin: await data.user.login,
                    userId: data.user.uid.toString(10),
                },
            } as OperationSchema);
            try {
                t.is(
                    await Activate4Customer.activateDevice4CustomerImplement(
                        device,
                        data.user,
                        data.headers,
                        data.operation.scope!,
                    ),
                    pendingOperation.id,
                );
                await sleep(100);
                t.true(t.context._getOwnerCode.notCalled);
                t.true(t.context._switchOwner.notCalled);
            } finally {
                await pendingOperation.destroy({ force: true });
            }
        } finally {
            await device.destroy({ force: true });
        }
    } finally {
        await organization.destroy({ force: true });
    }
});

test('throws _createOperationAndResetDevice exception', async (t) => {
    t.context._createMasterOperation.rejects(new Error('smth bad happen'));
    t.context._getOwnerCode.resolves('123');
    t.context._switchOwner.resolves();

    await t.throwsAsync(
        Activate4Customer.activateDevice4CustomerImplement(
            await Device.build(data.device),
            data.user,
            data.headers,
            data.operation.scope!,
        ),
        { message: 'smth bad happen' },
    );
    t.true(t.context._createMasterOperation.calledOnce);
    t.true(t.context._getOwnerCode.notCalled);
    t.true(t.context._switchOwner.notCalled);
});

test('ignores _getOwnerCode exception', async (t) => {
    const device = await Device.build(data.device);
    sinon.stub(device, "reload").resolves();
    const operation = await Operation.build({
        ...data.operation,
        type: OperationType.Activate,
        status: OperationStatus.Pending,
        device
    }, {
        include: [{model: Device, }]
    });
    sinon.stub(operation, "reload").resolves();
    t.context._createMasterOperation.resolves(operation);
    t.context._getOwnerCode.rejects(new Error('smth bad happen'));
    t.context._switchOwner.restore();
    const opId = await
        Activate4Customer.activateDevice4CustomerImplement(
            device,
            data.user,
            data.headers,
            data.operation.scope!,
        );
    await sleep(100);
    t.is(operation.id, opId);

    t.true(t.context._createMasterOperation.calledOnce);
    t.true(t.context._getOwnerCode.calledOnce);
    t.true(t.context._rejectOperation.calledOnce);
});

test('ignores _switchOwner exception', async (t) => {
    const device = await Device.build(data.device);
    sinon.stub(device, "reload").resolves();
    const operation = await Operation.build({
        ...data.operation,
        type: OperationType.Activate,
        status: OperationStatus.Pending,
        device
    }, {
        include: [{model: Device}]
    });
    sinon.stub(operation, "reload").resolves();
    t.context._createMasterOperation.resolves(operation);
    t.context._switchOwner.rejects(new Error('smth bad happen'));

    t.is(
        await Activate4Customer.activateDevice4CustomerImplement(
            device,
            data.user,
            data.headers,
            data.operation.scope!,
        ),
        operation.id,
    );

    await sleep(100);

    t.true(t.context._createMasterOperation.calledOnce);
    t.true(t.context._getOwnerCode.calledOnce);
    t.true(t.context._rejectOperation.calledOnce);
});

test('returns operation id', async (t) => {
    const device = await Device.build(data.device);
    sinon.stub(device, "reload").resolves();
    const operation = await Operation.build({
        ...data.operation,
        type: OperationType.Activate,
        status: OperationStatus.Pending,
        device
    }, 
    {
        include: [{model: Device}]
    });
    sinon.stub(operation, "reload").resolves();
    t.context._createMasterOperation.resolves(operation);
    t.context._getOwnerCode.resolves('123');
    t.context._switchOwner.resolves();

    t.is(
        await Activate4Customer.activateDevice4CustomerImplement(
            device,
            data.user,
            data.headers,
            data.operation.scope!,
        ),
        operation.id,
    );

    await sleep(1);

    t.true(t.context._createMasterOperation.calledOnce);
    t.true(t.context._getOwnerCode.calledOnce);
    t.true(t.context._switchOwner.calledOnce);
});

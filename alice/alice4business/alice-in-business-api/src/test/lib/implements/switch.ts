import anytest, { TestInterface } from 'ava';
import sinon from 'sinon';
import uniqueString from 'unique-string';
import { v4 as uuidv4 } from 'uuid';
import { Device, Operation } from '../../../db';
import { DeviceInstance, Status } from '../../../db/tables/device';
import { OperationInstance, Type as OperationType, Status as OperationStatus, OperationSchema } from '../../../db/tables/operation';
import { OrganizationInstance } from '../../../db/tables/organization';
import { RoomInstance } from '../../../db/tables/room';
import { createDevice, createOrganization, createRoom } from '../../helpers/db';
import * as PushSend from '../../../services/push/send';
import * as QuasarSwitch from '../../../services/quasar/switch';
import * as QuasarSync from '../../../lib/sync/quasar';
import { switchDevicesToUser, TargetUserInfo } from '../../../lib/implements/switch';

interface Context {
    organization: OrganizationInstance;
    device: DeviceInstance;
    device2: DeviceInstance;
    room: RoomInstance;
    operations: OperationInstance[];
    switchDeviceUser: sinon.SinonStub;
    syncDevice: sinon.SinonStub;
    sendPush: sinon.SinonStub;

}

const test = anytest as TestInterface<Context>;

test.beforeEach(async (t) => {
    t.context.organization = await createOrganization({
        id: uuidv4(),
        name: `${t.title} ${uniqueString()}`,
    });
    t.context.room = await createRoom({
        organizationId: t.context.organization.id
    });
    t.context.device = await createDevice({
        organizationId: t.context.organization.id,
        roomId: t.context.room.id,
        status: Status.Reset,

        kolonkishId: undefined,
        kolonkishLogin: undefined,
    });
    t.context.device2 = await createDevice({
        organizationId: t.context.organization.id,
        status: Status.Reset,
        roomId: t.context.room.id,

        kolonkishId: undefined,
        kolonkishLogin: undefined,
    });

    t.context.switchDeviceUser = sinon.stub(QuasarSwitch, 'switchDeviceUser').resolves();
    t.context.syncDevice = sinon.stub(QuasarSync, 'syncDevice').callsFake(async (device: DeviceInstance) => {
        await device.reload();
    })
    t.context.sendPush = sinon.stub(PushSend, 'sendPush').returns();
    await t.context.room.reload({ include: [Device] });
});

test.afterEach.always(async (t) => {
    sinon.restore();
    await t.context.device.destroy();
    await t.context.device2.destroy();
    await t.context.room.destroy();
    await t.context.organization.destroy();
});


test('Успешное переключение одного устройства с первой попытки логина', async (t) => {
    const targetUid = uniqueString();
    const code = uniqueString();
    const operation = await Operation.create({
        id: uuidv4(),
        devicePk: t.context.device.id,
        type: OperationType.Reset,
        status: OperationStatus.Pending
    } as OperationSchema);
    await operation.reload({ include: [Device] });

    let callNum = 0;

    t.context.syncDevice.callsFake(async (device: DeviceInstance): Promise<void> => {
        callNum++;
        if (callNum === 5) {
            await device.update({
                ownerId: targetUid
            });
        }
    })

    const generator = async () => {
        const xCodes = new Map<string, string>();
        xCodes.set(t.context.device.id, code);
        return {
            targetUid,
            targetLogin: uniqueString(),
            xCodes,
            isKolonkish: false,
        } as TargetUserInfo
    }


    await switchDevicesToUser(
        {
            entity: t.context.device,
            masterOperation: operation,
            deviceOperations: [operation],
            userInfoGenerator: generator,
            targetStatus: Status.Inactive,
            numSwitches: 3,
            numCompares: 10,
            sleepInterval: 0,
            metricSuffix: "reset",
            requireAll: true
        });

    await operation.reload({ include: [Device] });
    t.is(operation.status, OperationStatus.Resolved);
    t.is(t.context.switchDeviceUser.callCount, 1);
    t.is(t.context.syncDevice.callCount, 5);
    t.is(t.context.sendPush.callCount, 2); // устройство и комната
});


test('Успешное переключение комнаты', async (t) => {
    let targetUid = "";
    const code = uniqueString();
    const masterOperation = await Operation.create({
        id: uuidv4(),
        roomPk: t.context.room.id,
        type: OperationType.ResetRoom,
        status: OperationStatus.Pending
    } as OperationSchema);
    const d1Operation = await Operation.create({
        id: uuidv4(),
        devicePk: t.context.device.id,
        type: OperationType.Reset,
        parentId: masterOperation.id,
        status: OperationStatus.Pending
    } as OperationSchema);
    const d2Operation = await Operation.create({
        id: uuidv4(),
        devicePk: t.context.device2.id,
        type: OperationType.Reset,
        parentId: masterOperation.id,
        status: OperationStatus.Pending
    } as OperationSchema);
    await d1Operation.reload({ include: ["device", "parent"] });
    await d2Operation.reload({ include: ["device", "parent"] });
    await masterOperation.reload({ include: ["room", "children"] });

    const userDevices = new Map<string, string>([[t.context.device.id, ""], [t.context.device2.id, ""]]);
    const pollsPerDevice = new Map<string, number>();
    const switchesPerDevice = new Map<string, number>();


    t.context.switchDeviceUser.callsFake(async (deviceId: string) => {
        const switchNum = (switchesPerDevice.get(deviceId) || 0) + 1;
        switchesPerDevice.set(deviceId, switchNum);
        pollsPerDevice.set(deviceId, 0);
        switch (deviceId) {
            case t.context.device.deviceId:
                userDevices.set(deviceId, targetUid);
                break;
            case t.context.device2.deviceId:
                if (switchNum > 1) {
                    userDevices.set(deviceId, targetUid);
                }
                break;
        }
    });

    t.context.syncDevice.callsFake(async (device: DeviceInstance): Promise<void> => {
        const pollNum = (pollsPerDevice.get(device.deviceId) || 0) + 1;
        const switchNum = (switchesPerDevice.get(device.deviceId) || 0);
        pollsPerDevice.set(device.deviceId, pollNum);
        if (pollNum > 3) {
            const userId = userDevices.get(device.deviceId);
            await device.update({
                ownerId: userId,
            });
        }
    })

    const generator = async () => {
        const xCodes = new Map<string, string>();
        targetUid = uniqueString();
        xCodes.set(t.context.device.id, code);
        xCodes.set(t.context.device2.id, code);
        return {
            targetUid,
            targetLogin: uniqueString(),
            xCodes,
            isKolonkish: false,
        } as TargetUserInfo
    }


    await switchDevicesToUser(
        {
            entity: t.context.room,
            masterOperation,
            deviceOperations: [d1Operation, d2Operation],
            userInfoGenerator: generator,
            targetStatus: Status.Inactive,
            numSwitches: 3,
            numCompares: 10,
            sleepInterval: 0,
            metricSuffix: "reset",
            requireAll: true
        });

    await masterOperation.reload({ include: [Device] });
    t.is(masterOperation.status, OperationStatus.Resolved);
    // Первая попытка переключения: устройство 1 обнаруживает правильный логин за 4 полла, второе - нет (проходит 10 поллова)
    // Вторая попытка переключение: каждое устройство с 4 поллов находит свой логин
    // Итого 4+10+4+4
    t.is(t.context.syncDevice.callCount, 22);
    t.is(t.context.switchDeviceUser.callCount, 4); // 2 устройства за 2 попытки
    t.is(t.context.sendPush.callCount, 3); // 2 устройства + 1 комната
});

test('Ошибка переключения комнаты — одно из устройств всегда offline', async (t) => {
    let targetUid = uniqueString();
    const code = uniqueString();
    const masterOperation = await Operation.create({
        id: uuidv4(),
        roomPk: t.context.room.id,
        type: OperationType.ResetRoom,
        status: OperationStatus.Pending
    } as OperationSchema);
    const d1Operation = await Operation.create({
        id: uuidv4(),
        devicePk: t.context.device.id,
        type: OperationType.Reset,
        parentId: masterOperation.id,
        status: OperationStatus.Pending
    } as OperationSchema);
    const d2Operation = await Operation.create({
        id: uuidv4(),
        devicePk: t.context.device2.id,
        type: OperationType.Reset,
        parentId: masterOperation.id,
        status: OperationStatus.Pending
    } as OperationSchema);
    await d1Operation.reload({ include: ["device", "parent"] });
    await d2Operation.reload({ include: ["device", "parent"] });
    await masterOperation.reload({ include: ["room", "children"] });

    const userDevices = new Map<string, string>([[t.context.device.id, ""], [t.context.device2.id, ""]]);

    t.context.switchDeviceUser.callsFake(async (deviceId: string) => {
        if (deviceId === t.context.device.deviceId) {
            userDevices.set(deviceId, targetUid);
        }
    });

    t.context.syncDevice.callsFake(async (device: DeviceInstance): Promise<void> => {
        const userId = userDevices.get(device.deviceId);
        await device.update({
            ownerId: userId,
        });

    })

    const generator = async () => {
        const xCodes = new Map<string, string>();
        targetUid = uniqueString();
        xCodes.set(t.context.device.id, code);
        xCodes.set(t.context.device2.id, code);
        return {
            targetUid,
            targetLogin: uniqueString(),
            xCodes,
            isKolonkish: false,
        } as TargetUserInfo
    }


    await switchDevicesToUser({
        entity: t.context.room,
        masterOperation,
        deviceOperations: [d1Operation, d2Operation],
        userInfoGenerator: generator,
        targetStatus: Status.Inactive,
        numSwitches: 3,
        numCompares: 10,
        sleepInterval: 0,
        metricSuffix: "reset",
        requireAll: true
    });

    await masterOperation.reload({ include: ["device", "children"] });
    t.is(masterOperation.status, OperationStatus.Rejected);
    t.is(masterOperation.error.message, "No more retries");
    masterOperation.children!.forEach(c => t.is(c.status, OperationStatus.Rejected));
    masterOperation.children!.forEach(c => t.is(c.error.message, "No more retries"));
    t.is(t.context.switchDeviceUser.callCount, 6); // 2 устройства x 3 попытки
    t.is(t.context.syncDevice.callCount, 33); // 10 поллов на попытку у неуспешного устройства, 1 - у успешного. Итого (10+1)x3 = 33 полла
    t.is(t.context.sendPush.callCount, 3); // 2 устройства + 1 комната
});

test('Частичное переключение комнаты — одно из устройств всегда offline', async (t) => {
    let targetUid = uniqueString();
    const code = uniqueString();
    const masterOperation = await Operation.create({
        id: uuidv4(),
        roomPk: t.context.room.id,
        type: OperationType.ResetRoom,
        status: OperationStatus.Pending
    } as OperationSchema);
    const d1Operation = await Operation.create({
        id: uuidv4(),
        devicePk: t.context.device.id,
        type: OperationType.Reset,
        parentId: masterOperation.id,
        status: OperationStatus.Pending
    } as OperationSchema);
    const d2Operation = await Operation.create({
        id: uuidv4(),
        devicePk: t.context.device2.id,
        type: OperationType.Reset,
        parentId: masterOperation.id,
        status: OperationStatus.Pending
    } as OperationSchema);
    await d1Operation.reload({ include: ["device", "parent"] });
    await d2Operation.reload({ include: ["device", "parent"] });
    await masterOperation.reload({ include: ["room", "children"] });

    const userDevices = new Map<string, string>([[t.context.device.id, ""], [t.context.device2.id, ""]]);

    t.context.switchDeviceUser.callsFake(async (deviceId: string) => {
        if (deviceId === t.context.device.deviceId) {
            userDevices.set(deviceId, targetUid);
        }
    });

    t.context.syncDevice.callsFake(async (device: DeviceInstance): Promise<void> => {
        const userId = userDevices.get(device.deviceId);
        await device.update({
            ownerId: userId,
        });

    })

    const generator = async () => {
        const xCodes = new Map<string, string>();
        targetUid = uniqueString();
        xCodes.set(t.context.device.id, code);
        xCodes.set(t.context.device2.id, code);
        return {
            targetUid,
            targetLogin: uniqueString(),
            xCodes,
            isKolonkish: false,
        } as TargetUserInfo
    }


    await switchDevicesToUser({
        entity: t.context.room,
        masterOperation,
        deviceOperations: [d1Operation, d2Operation],
        userInfoGenerator: generator,
        targetStatus: Status.Active,
        numSwitches: 3,
        numCompares: 10,
        sleepInterval: 0,
        metricSuffix: "reset",
        requireAll: false
    });

    await masterOperation.reload({ include: ["device", "children"] });
    await d1Operation.reload({ include: ["device"] });
    await d2Operation.reload({ include: ["device"] });
    t.is(masterOperation.status, OperationStatus.Resolved);
    t.is(d1Operation.status, OperationStatus.Resolved);
    t.is(d2Operation.status, OperationStatus.Rejected);
    t.is(d2Operation.error.message, "No more retries");
    t.is(d1Operation.device!.status, Status.Active);
    t.is(d2Operation.device!.status, Status.Reset);
    t.is(t.context.switchDeviceUser.callCount, 2); // 2 устройства x 1 попытка
    t.is(t.context.syncDevice.callCount, 11); // 10 поллов на попытку у неуспешного устройства, 1 - у успешного. Итого 11 полла
    t.is(t.context.sendPush.callCount, 3); // 2 устройства + 1 комната
});


test('Ошибка переключения комнаты — квазар возвращает ошибку для одного из пушей', async (t) => {
    const targetUid = uniqueString();
    const code = uniqueString();
    const masterOperation = await Operation.create({
        id: uuidv4(),
        roomPk: t.context.room.id,
        type: OperationType.ResetRoom,
        status: OperationStatus.Pending
    } as OperationSchema);
    const d1Operation = await Operation.create({
        id: uuidv4(),
        devicePk: t.context.device.id,
        type: OperationType.Reset,
        parentId: masterOperation.id,
        status: OperationStatus.Pending
    } as OperationSchema);
    const d2Operation = await Operation.create({
        id: uuidv4(),
        devicePk: t.context.device2.id,
        type: OperationType.Reset,
        parentId: masterOperation.id,
        status: OperationStatus.Pending
    } as OperationSchema);
    await d1Operation.reload({ include: ["device", "parent"] });
    await d2Operation.reload({ include: ["device", "parent"] });
    await masterOperation.reload({ include: ["room", "children"] });

    const userDevices = new Map<string, string>([[t.context.device.id, ""], [t.context.device2.id, ""]]);

    t.context.switchDeviceUser.callsFake(async (deviceId: string) => {
        if (deviceId === t.context.device.deviceId) {
            userDevices.set(deviceId, targetUid);
        } else {
            throw new Error("ошибка в квазаре");
        }
    });

    t.context.syncDevice.callsFake(async (device: DeviceInstance): Promise<void> => {
        const userId = userDevices.get(device.deviceId);
        await device.update({
            ownerId: userId,
        });

    })

    const generator = async () => {
        const xCodes = new Map<string, string>();
        xCodes.set(t.context.device.id, code);
        xCodes.set(t.context.device2.id, code);
        return {
            targetUid,
            targetLogin: uniqueString(),
            xCodes,
            isKolonkish: false,
        } as TargetUserInfo
    }


    await switchDevicesToUser({
        entity: t.context.room,
        masterOperation,
        deviceOperations: [d1Operation, d2Operation],
        userInfoGenerator: generator,
        targetStatus: Status.Inactive,
        numSwitches: 3,
        numCompares: 10,
        sleepInterval: 0,
        metricSuffix: "reset",
        requireAll: true
    });

    await masterOperation.reload({ include: ["device", "children"] });
    t.is(masterOperation.status, OperationStatus.Rejected);
    t.is(masterOperation.error.message, "ошибка в квазаре");
    masterOperation.children!.forEach(c => t.is(c.status, OperationStatus.Rejected));
    masterOperation.children!.forEach(c => t.is(c.error.message, "ошибка в квазаре"));
    t.is(t.context.switchDeviceUser.callCount, 2); // 2 устройства x 1 попытка
    t.false(t.context.syncDevice.called); // до поллов дело не дошло
    t.is(t.context.sendPush.callCount, 3); // 2 устройства + 1 комната
});
import { Op, Transaction } from "sequelize";
import { Device, sequelize } from "../../db";
import { DeviceInstance, DeviceSchema, Status as DeviceStatus } from "../../db/tables/device";
import { OperationInstance, OperationSchema, Status as OperationStatus } from "../../db/tables/operation";
import { RoomInstance } from "../../db/tables/room";
import { notifyStateChange } from "../../services/push/send";
import { switchDeviceUser } from "../../services/quasar/switch";
import * as solomon from '../../services/solomon';
import log from "../log";
import { syncDevice } from "../sync/quasar";
import { sleep } from "../utils";
import { logOperationMessage, rejectOperation, updateOperationsLastHandling, updatePendingOperation } from "./operations";

export interface TargetUserInfo {
    targetUid: string,
    targetLogin: string,
    xCodes: Map<string, string>,
    isKolonkish: boolean,
}

export interface SwitchParams {
    entity: RoomInstance | DeviceInstance,
    masterOperation: OperationInstance,
    deviceOperations: OperationInstance[],
    userInfoGenerator: () => Promise<TargetUserInfo>,
    targetStatus: DeviceStatus,
    numSwitches: number,
    numCompares: number,
    sleepInterval: number,
    metricSuffix: 'switch' | 'reset',
    requireAll: boolean,
}

export const switchDevicesToUser = async (
    {
        entity,
        masterOperation,
        deviceOperations,
        userInfoGenerator,
        targetStatus,
        numSwitches,
        numCompares,
        sleepInterval,
        metricSuffix,
        requireAll
    }: SwitchParams
) => {
    const ts1 = Date.now();
    let compareCounter = 0;
    let switchCounter = 0;

    const operationMap = new Map<string, OperationInstance>();
    const operationStatusMap = new Map<string, boolean>();
    deviceOperations.forEach(operation => {
        operationStatusMap.set(operation.id, false);
        operationMap.set(operation.id, operation)
    });
    let targetUid = '';
    let targetUserInfo: TargetUserInfo;
    try {
        for (
            let switchRetriesLeft = numSwitches;
            switchRetriesLeft > 0;
            --switchRetriesLeft
        ) {
            switchCounter++;
            const ts2 = Date.now();
            const allOps = entity instanceof RoomInstance ? [masterOperation, ...deviceOperations] : deviceOperations
            await updateOperationsLastHandling(allOps);
            logOperationMessage(log.debug, masterOperation, `switching user, ${switchRetriesLeft} retries left, will${requireAll ? "" : " NOT"} wait for all devices to switch`);
            // (re)generating activation x-codes
            targetUserInfo = await userInfoGenerator();
            if (targetUserInfo.targetUid !== targetUid) {
                deviceOperations.forEach(operation => operationStatusMap.set(operation.id, false));
                targetUid = targetUserInfo.targetUid;
            }
            // sending pushes to all devices
            await Promise.all(deviceOperations.map(async operation => {
                const code = targetUserInfo.xCodes.get(operation.device!.id);
                if (!code) {
                    throw new Error(`Unable to generate x-code for operation ${operation.id}`);
                }
                return switchDeviceUser(
                    operation.device!.deviceId,
                    operation.device!.platform,
                    code
                );
            }));
            let allSwitched = false;
            let someSwitched = false;
            for (
                let compareRetries = numCompares;
                compareRetries > 0;
                --compareRetries
            ) {
                compareCounter++;
                const ts3 = Date.now();
                await sleep(sleepInterval);
                await updateOperationsLastHandling(allOps);
                logOperationMessage(log.debug, masterOperation, `check device(s) owner, ${compareRetries} retries left`);
                await Promise.all(deviceOperations.map(async operation => {
                    const alreadySwitched = operationStatusMap.get(operation.id);
                    if (!alreadySwitched) {
                        const device = operation.device!;
                        await syncDevice(device);
                        if (device.ownerId === targetUserInfo.targetUid) {
                            logOperationMessage(log.debug, operation, "owner successfully changed to target uid");
                            someSwitched = true;
                            operationStatusMap.set(operation.id, true);
                        }
                    }
                }));
                solomon.addSwitchMetric('compare_timing', Date.now() - ts3);
                if (Array.from(operationStatusMap.values()).every(v => v)) {
                    logOperationMessage(log.debug, masterOperation, `owner successfully changed to target uid on all required devices`);
                    allSwitched = true
                    break;
                }
            }
            solomon.addSwitchMetric('switch_timing', Date.now() - ts2);
            if (allSwitched) {
                await resolveOperations(entity instanceof RoomInstance ? masterOperation : null, deviceOperations, targetStatus, targetUserInfo);

                // Solomon metrics
                solomon.incCounter('switchCounter', solomon.switchCounters, switchCounter);
                solomon.incCounter('compareCounter', solomon.switchCounters, compareCounter);
                const workTime = Date.now() - ts1;
                if (metricSuffix === 'switch') {
                    solomon.incCounter(`device__switch_success`, undefined, deviceOperations.length);
                    solomon.addSwitchMetric('device__switch_success_timing', workTime)
                } else if (metricSuffix === 'reset') {
                    solomon.incCounter(`device__reset_success`, undefined, deviceOperations.length);
                    solomon.addSwitchMetric('device__reset_success_timing', workTime)
                }
                if (entity instanceof RoomInstance) {
                    if (metricSuffix === 'switch') {
                        solomon.incCounter(`room__switch_success`);
                        solomon.addSwitchMetric('room__switch_success_timing', workTime)
                    } else if (metricSuffix === 'reset') {
                        solomon.incCounter(`room__reset_success`);
                        solomon.addSwitchMetric('room__reset_success_timing', workTime)
                    }
                }

                notifyStateChange(entity);
                logOperationMessage(log.info, masterOperation, "done");
                return;
            } else {
                if (someSwitched && !requireAll) {
                    logOperationMessage(log.info, masterOperation, "some devices switched successfully, will not wait for others");
                    break;
                }
                logOperationMessage(log.info, masterOperation, "retrying from the beginning");
            }
        }
        if (requireAll) {
            throw new Error("No more retries");
        } else {
            const switchedOps = [] as OperationInstance[];
            const failedOps = [] as OperationInstance[];

            for (const [id, status] of operationStatusMap) {
                if (status) {
                    switchedOps.push(operationMap.get(id)!);
                } else {
                    failedOps.push(operationMap.get(id)!);
                }
            }
            logOperationMessage(log.info, masterOperation, `${switchedOps.length} operations have switched successfully, ${failedOps.length} have failed"`);
            if (switchedOps.length === 0) {
                throw new Error("No more retries");
            }
            await sequelize.transaction(async transaction => {
                if (requireAll) {
                    await resolveOperationsInTransaction(null, switchedOps, targetStatus, targetUserInfo, transaction);
                    await rejectOperation(masterOperation, new Error("Some devices failed to switch"), transaction, false);
                } else {
                    await resolveOperationsInTransaction(masterOperation, switchedOps, targetStatus, targetUserInfo, transaction)
                }
                await Promise.all(failedOps.map(operation => rejectOperation(operation, new Error("No more retries"), transaction, false)));
            });
            notifyStateChange(entity);
        }
    } catch (error) {
        // Solomon metrics
        solomon.incCounter('switchCounter', solomon.switchCounters, switchCounter);
        solomon.incCounter('compareCounter', solomon.switchCounters, compareCounter);
        const workTime = Date.now() - ts1;
        if (metricSuffix === 'switch') {
            solomon.incCounter(`device__switch_error`, undefined, deviceOperations.length);
            solomon.addSwitchMetric('device__switch_error_timing', workTime)
        } else if (metricSuffix === 'reset') {
            solomon.incCounter(`device__reset_error`, undefined, deviceOperations.length);
            solomon.addSwitchMetric('device__reset_error_timing', workTime)
        }
        if (entity instanceof RoomInstance) {
            if (metricSuffix === 'switch') {
                solomon.incCounter(`room__switch_error`);
                solomon.addSwitchMetric('room__switch_error_timing', workTime)
            } else if (metricSuffix === 'reset') {
                solomon.incCounter(`room__reset_error`);
                solomon.addSwitchMetric('room__switch_error_timing', workTime)
            }
        }
        logOperationMessage(log.error, masterOperation, "failed", undefined, error);
        await masterOperation.reload({ include: ["children"] });
        await rejectOperation(masterOperation, error);
        notifyStateChange(entity);
    }
}

const resolveOperations = async (masterOperation: OperationInstance | null, deviceOperations: OperationInstance[], targetStatus: DeviceStatus, targetUserInfo: TargetUserInfo) => {
    await sequelize.transaction(async transaction =>
        await resolveOperationsInTransaction(masterOperation, deviceOperations, targetStatus, targetUserInfo, transaction));
}

const resolveOperationsInTransaction = async (masterOperation: OperationInstance | null, deviceOperations: OperationInstance[], targetStatus: DeviceStatus, targetUserInfo: TargetUserInfo, transaction: Transaction) => {
    const deviceIds = deviceOperations.map(operation => operation.device!.id);
    await Device.update(
        {
            status: targetStatus,
            kolonkishId: targetUserInfo.isKolonkish ? targetUserInfo.targetUid : null,
            kolonkishLogin: targetUserInfo.isKolonkish ? targetUserInfo.targetLogin : null,
            agreementAccepted: targetUserInfo.isKolonkish ? false : true,
        } as DeviceSchema, {
        where: {
            id: {
                [Op.in]: deviceIds,
            }
        },
        transaction,
    });
    const operationsToComplete = masterOperation ? [masterOperation, ...deviceOperations] : deviceOperations;
    return Promise.all(operationsToComplete.map(async operation => await updatePendingOperation(operation, {
        status: OperationStatus.Resolved
    } as OperationSchema, false, transaction)));
}

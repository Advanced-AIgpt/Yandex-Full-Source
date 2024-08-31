import { DeviceInstance, DeviceSchema, Status as DeviceStatus } from '../../db/tables/device';
import { IncomingHttpHeaders } from 'http';
import RestApiError from '../errors/restApi';
import { Organization, Device } from '../../db';
import {
    OperationInstance,
    OperationSchema,
    Scope as OperationScope,
    Type as OperationType,
} from '../../db/tables/operation';
import log from '../log';
import { KolonkishResponse, multiply, registryKolonkish } from '../../services/passport/kolonkish';
import config from '../config';
import { RoomInstance } from '../../db/tables/room';
import { createOperation, logOperationMessage, OperationAlreadyRunning, rejectOperation, updatePendingOperation } from '../../lib/implements/operations';
import { notifyStateChange } from '../../services/push/send';
import { switchDevicesToUser } from './switch';


export const resetDeviceImplement = async (
    device: DeviceInstance,
    clientIp: string,
    headers: IncomingHttpHeaders,
    scope: OperationScope,
) => {
    return resetBatch(device, clientIp, headers, scope, true);
};

export const resetRoomImplement = async (
    room: RoomInstance,
    clientIp: string,
    headers: IncomingHttpHeaders,
    scope: OperationScope,
    requireAll: boolean
) => {
    return resetBatch(room, clientIp, headers, scope, requireAll);
}

interface DeviceKolonkishes {
    stationKolonkishes: KolonkishResponse[],
    tvKolonkishes: KolonkishResponse[],
}

const resetBatch = async (
    roomOrDevice: RoomInstance | DeviceInstance,
    clientIp: string,
    headers: IncomingHttpHeaders,
    scope: OperationScope,
    requireAll: boolean,
) => {
    try {
        const masterOperation = await createOperation(roomOrDevice, scope, OperationType.Reset, {
            status: DeviceStatus.Reset,
            kolonkishId: null,
            kolonkishLogin: null,
            agreementAccepted: false,
        } as DeviceSchema);
        if (!roomOrDevice.organization) {
            await roomOrDevice.reload({ include: [Organization] });
        }
        notifyStateChange(roomOrDevice);
        const deviceOperations = roomOrDevice instanceof RoomInstance ? masterOperation.children! : [masterOperation];
        await Promise.all(deviceOperations.map(async operation => {
            if (!operation.device) {
                await operation.reload({ include: [Device] });
            }
        }));
        const roomName = roomOrDevice instanceof RoomInstance ? roomOrDevice.name : roomOrDevice.room?.name;
        const displayName = roomName ? `${roomOrDevice.organization!.name} — ${roomName}` : roomOrDevice.organization!.name;
        const masterKolonkish = await registerMasterKolonkish(headers, clientIp, displayName, masterOperation)
            .catch(async (registerKolonkishError) => {
                const apiError = (registerKolonkishError instanceof RestApiError)
                    ? registerKolonkishError
                    : new RestApiError('Failed to register account', 500, {
                        origError: registerKolonkishError,
                    });
                logOperationMessage(log.warn, masterOperation, 'failed to register kolonkish', undefined, registerKolonkishError)
                await rejectOperation(masterOperation, apiError);
                notifyStateChange(roomOrDevice);
                throw apiError;
            });
        setImmediate(async () => {
            const getUserInfo = async () => {
                const xCodes: Map<string, string> = new Map();
                const deviceKolonkishes = await buildKolonkishes(roomOrDevice, masterOperation, masterKolonkish, clientIp);
                await Promise.all(deviceOperations.map(async operation => {
                    const kolonkish = operation.device!.isTv() ? deviceKolonkishes.tvKolonkishes.pop() : deviceKolonkishes.stationKolonkishes.pop();
                    if (!kolonkish) {
                        throw new Error(`No kolonkish for ${operation.device!.isTv() ? 'tv' : 'station'} device`);
                    }
                    await updatePendingOperation(operation, {
                        payload: { kolonkishUid: kolonkish!.uid, kolonkishLogin: kolonkish!.login },
                        lastHandling: new Date(),
                    } as OperationSchema);
                    xCodes.set(operation.device!.id, kolonkish.code);
                    logOperationMessage(log.info, operation, "kolonkish registered", { kolonkish: { uid: kolonkish.uid, login: kolonkish.login } });
                })).catch(async (registerKolonkishError) => {
                    logOperationMessage(log.warn, masterOperation, 'failed to update operation with kolonkish', undefined, registerKolonkishError)
                    throw registerKolonkishError;
                });
                logOperationMessage(log.info, masterOperation, "all kolonkishes registered");
                return {
                    targetUid: masterKolonkish.uid,
                    targetLogin: masterKolonkish.login,
                    isKolonkish: true,
                    xCodes
                }
            }
            return switchDevicesToUser(
                {
                    entity: roomOrDevice,
                    masterOperation,
                    deviceOperations,
                    userInfoGenerator: getUserInfo,
                    targetStatus: DeviceStatus.Inactive,
                    numSwitches: config.app.resetOperation.switchRetries,
                    numCompares: config.app.resetOperation.compareRetries,
                    sleepInterval: config.app.resetOperation.compareInterval,
                    metricSuffix: "reset",
                    requireAll,
                });
        });
        return masterOperation.id;
    } catch (resetError) {
        if (resetError instanceof OperationAlreadyRunning) {
            return resetError.existing.id;
        } else {
            throw resetError;
        }
    }
};

const registerMasterKolonkish = async (headers: IncomingHttpHeaders, clientIp: string, displayName: string, masterOperation: OperationInstance) => {
    return await registryKolonkish({
        OAuth: headers.authorization,
        cookie: headers.cookie,
        clientIp,
        displayName,
    })
        .then(async r => {
            logOperationMessage(log.info, masterOperation, 'registered master-kolonkish', r)
            await updatePendingOperation(masterOperation, {
                payload: { kolonkishUid: r!.uid, kolonkishLogin: r!.login },
                lastHandling: new Date(),
            } as OperationSchema);
            return r;
        })
        .catch(async (registerKolonkishError) => {
            logOperationMessage(log.warn, masterOperation, 'failed to create and register master kolonkish', undefined, registerKolonkishError);
            throw registerKolonkishError;
        });
}

const buildKolonkishes = async (entity: RoomInstance | DeviceInstance, operation: OperationInstance, masterKolonkish: KolonkishResponse, clientIp: string) => {
    let result: DeviceKolonkishes
    if (entity instanceof RoomInstance) {
        result = await buildKolonkishesForRoom(entity, operation, masterKolonkish, clientIp)
    } else {
        result = await buildKolonkishForSingleDevice(entity, operation, masterKolonkish, clientIp);
    }
    logOperationMessage(log.info, operation, 'got device kolonkishes', result);
    return result
}

const buildKolonkishesForRoom = async (room: RoomInstance, operation: OperationInstance, masterKolonkish: KolonkishResponse, clientIp: string) => {
    const result = {
        stationKolonkishes: [] as KolonkishResponse[],
        tvKolonkishes: [] as KolonkishResponse[]
    }

    const numStations = room.devices!.filter(device => device.isStation()).length;
    const numTvs = room.devices!.filter(device => device.isTv()).length;
    result.stationKolonkishes = await multiply(masterKolonkish, clientIp, numStations, false)
        .catch(async (mutiplyKolonkishError) => {
            const apiError = new RestApiError('Failed to multiply account', 500, {
                origError: mutiplyKolonkishError,
            });
            logOperationMessage(log.warn, operation, 'failed to multiply kolonkish', undefined, mutiplyKolonkishError)
            await rejectOperation(operation, apiError);
            notifyStateChange(room)
            throw apiError;
        });
    result.tvKolonkishes = await multiply(masterKolonkish, clientIp, numTvs, true)
        .catch(async (mutiplyKolonkishError) => {
            const apiError = new RestApiError('Failed to multiply account', 500, {
                origError: mutiplyKolonkishError,
            });
            logOperationMessage(log.warn, operation, 'failed to multiply kolonkish', undefined, mutiplyKolonkishError)
            await rejectOperation(operation, apiError);
            notifyStateChange(room)
            throw apiError;
        });
    return result
}

const buildKolonkishForSingleDevice = async (device: DeviceInstance, operation: OperationInstance, masterKolonkish: KolonkishResponse, clientIp: string) => {
    const multiplied = await multiply(masterKolonkish, clientIp, 1, device.isTv())
        .catch(async (multiplyKolonkishError) => {
            const apiError = new RestApiError('Failed to multiply account', 500, {
                origError: multiplyKolonkishError,
            });
            logOperationMessage(log.warn, operation, 'failed to multiply kolonkish', undefined, multiplyKolonkishError)
            await rejectOperation(operation, apiError);
            notifyStateChange(device)
            throw apiError;
        });

    if (device.isTv()) {
        return {
            stationKolonkishes: [],
            tvKolonkishes: multiplied
        }
    } else {
        return {
            stationKolonkishes: multiplied,
            tvKolonkishes: []
        }
    }
}

import { DeviceInstance, DeviceSchema, Status as DeviceStatus } from '../../db/tables/device';
import { IncomingHttpHeaders } from 'http';
import RestApiError from '../errors/restApi';
import { Device, Organization } from '../../db';
import {
    OperationInstance,
    Scope as OperationScope,
    Type as OperationType,
} from '../../db/tables/operation';
import log from '../log';
import { notifyStateChange, sendPush } from '../../services/push/send';
import * as ACL from '../acl';
import { getCodeForAm } from '../../services/passport/code';
import { createOperation, OperationAlreadyRunning, rejectOperation } from './operations';
import config from '../config';
import { RoomInstance } from '../../db/tables/room';
import { switchDevicesToUser } from './switch';

export const activateDevice4CustomerImplement = async (
    roomOrDevice: DeviceInstance | RoomInstance,
    user: ACL.User,
    headers: IncomingHttpHeaders,
    scope: OperationScope,
) => {
    try {
        const masterOperation = await createOperation(roomOrDevice, scope, OperationType.Activate, {
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
        setImmediate(async () => {
            const getUserInfo = async () => {
                const xCodes = new Map<string, string>();
                await Promise.all(deviceOperations.map(async operation => {
                    const ownerCode = await _getOwnerCode(operation, operation.device!, user, headers);
                    xCodes.set(operation.device!.id, ownerCode);
                }));
                return {
                    targetUid: user.uid.toString(10),
                    targetLogin: "",
                    isKolonkish: false,
                    xCodes
                }
            }
            return switchDevicesToUser(
                {
                    entity: roomOrDevice,
                    masterOperation,
                    deviceOperations,
                    userInfoGenerator: getUserInfo,
                    targetStatus: DeviceStatus.Active,
                    numSwitches: config.app.customerActivationOperation.switchRetries,
                    numCompares: config.app.customerActivationOperation.compareRetries,
                    sleepInterval: config.app.customerActivationOperation.compareInterval,
                    metricSuffix: "switch",
                    requireAll: false,
                }
            );
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

export const _getOwnerCode = async (
    operation: OperationInstance,
    device: DeviceInstance,
    user: ACL.User,
    headers: IncomingHttpHeaders,
) => {
    try {
        const ownerCode = await getCodeForAm({
            OAuth: headers.authorization,
            cookie: headers.cookie,
            clientIp: user.ip,
            isTv: device.isTv()
        });

        if (!ownerCode) {
            throw new Error('x-code is empty');
        }

        log.info('activate device for customer: got x-code', {
            activateDevice4Customer: {
                device: device.id,
                uid: user.uid,
                operation: operation.id,
            },
        });

        return ownerCode;
    } catch (getCodeError) {
        log.warn('activate device for customer: failed to get x-code', {
            error: getCodeError,
            activateDevice4Customer: {
                device: device.id,
                uid: user.uid,
                operation: operation.id,
            },
        });

        const error = new RestApiError('Failed to get auth code', 500, {
            origError: getCodeError,
        });

        await rejectOperation(operation, error);
        sendPush({
            topic: device.organizationId,
            event: 'device-state',
            payload: device.id,
        });

        throw error;
    }
};

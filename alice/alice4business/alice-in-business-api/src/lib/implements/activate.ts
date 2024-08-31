import { Op, Transaction, WhereValue } from 'sequelize';
import { DeviceInstance, DeviceSchema, Status } from '../../db/tables/device';
import RestApiError from '../errors/restApi';
import { sequelize, Device, Operation } from '../../db';
import {
    OperationInstance,
    OperationSchema,
    Scope as OperationScope,
    Status as OperationStatus,
    Type as OperationType,
} from '../../db/tables/operation';
import log from '../log';
import { RoomInstance } from '../../db/tables/room';
import { createOperation, logOperationMessage, OperationAlreadyRunning, rejectOperation, updatePendingOperation } from './operations';
import { notifyStateChange } from '../../services/push/send';
import * as solomon from '../../services/solomon';


const activateSingle = async (operation: OperationInstance, transaction: Transaction) => {
    await Device.update(
        {
            status: Status.Active,
        } as DeviceSchema,
        {
            where: {
                id: operation.devicePk!,
                status: Status.Inactive,
                kolonkishId: { [Op.ne]: null },
            },
            transaction
        }
    ).then(([count]) => {
        if (count !== 1) {
            throw new RestApiError('Device status is not Inactive', 409, {
                payload: {
                    text: 'Для разблокировки устройства обратитесь на ресепшен',
                },
            });
        }
    });

    await Operation.update(
        {
            status: OperationStatus.Resolved,
        } as OperationSchema,
        {
            where: {
                id: operation.id,

                status: OperationStatus.Pending,
                updatedAt: operation.updatedAt as WhereValue,
            },
            transaction,
        },
    ).then(([updatedOperationCount]) => {
        if (updatedOperationCount !== 1) {
            throw new RestApiError('Operation was cancelled', 409);
        }
    });
}

export const activateImplement = async (
    entity: DeviceInstance | RoomInstance,
    scope: OperationScope,
): Promise<string> => {
    const ts = Date.now();
    let masterOperation: OperationInstance
    try {
        masterOperation = await createOperation(entity, scope, OperationType.Activate, undefined, device => device.status === Status.Inactive);
    } catch (activateError) {
        if (activateError instanceof OperationAlreadyRunning) {
            return activateError.existing.id;
        } else {
            throw RestApiError.fromError(activateError);
        }
    }
    try {
        if (entity instanceof RoomInstance) {
            await masterOperation.reload({ include: [{ model: Operation, as: "children", include: [{ model: Device }] }] })
            await Promise.all(masterOperation.children!.map(async operation => operation.update({
                payload: operation.device?.kolonkishId && {
                    kolonkishLogin: operation.device.kolonkishLogin,
                    kolonkishUid: operation.device.kolonkishId,
                }
            })));
        } else {
            await masterOperation.update({
                payload: entity.kolonkishId && {
                    kolonkishLogin: entity.kolonkishLogin,
                    kolonkishUid: entity.kolonkishId,
                }
            });
        }
        notifyStateChange(entity);
        logOperationMessage(log.info, masterOperation, 'operation created');
        const deviceOperations = entity instanceof RoomInstance ? masterOperation.children! : [masterOperation];

        await sequelize
            .transaction(async (transaction) => {
                await Promise.all(deviceOperations.map(
                    async device => await activateSingle(device, transaction)
                        .catch(async (singleOpError) => {
                            logOperationMessage(log.warn, device, 'activation failed', undefined, singleOpError);
                            // await rejectOperation(d, singleOpError); no need to reject, as master operation will be rejected in catch
                            throw singleOpError;
                        })))
                    .then(async _ => {
                        if (entity instanceof RoomInstance) {
                            const now = new Date();
                            await updatePendingOperation(masterOperation, {
                                status: OperationStatus.Resolved,
                                lastHandling: now,
                                updatedAt: now
                            } as OperationSchema, true, transaction);
                        }
                        solomon.addSwitchMetric('guest_activate_timing', Date.now() - ts);
                        notifyStateChange(entity);
                    })
            });
        return masterOperation.id;
    } catch (activateError) {
        logOperationMessage(log.warn, masterOperation, 'operation failed', undefined, activateError)
        notifyStateChange(entity);
        await rejectOperation(masterOperation, activateError)
        throw RestApiError.fromError(activateError);
    }
};

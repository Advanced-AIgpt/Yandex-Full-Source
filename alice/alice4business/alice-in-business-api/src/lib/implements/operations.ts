import { DeviceInstance, DeviceSchema } from '../../db/tables/device';
import { OperationInstance, OperationSchema, Scope, Status as OperationStatus, Type as OperationType } from '../../db/tables/operation';
import { RoomInstance } from '../../db/tables/room';
import { sequelize, Operation, Device } from '../../db';
import { isSequelizeUniqueConstraintError, trimPrivateFields } from '../errors/utils';
import RestApiError from '../errors/restApi';
import { Op, Transaction, WhereOptions, WhereValue } from 'sequelize';
import { serializeError } from 'serialize-error';
import * as solomon from '../../services/solomon';
import log from '../log';
import { LogFunction } from 'kroniko';

export const batchTypeMapping: Map<OperationType, OperationType> = new Map([
    [OperationType.Activate, OperationType.ActivateRoom],
    [OperationType.Reset, OperationType.ResetRoom],
]);

const operationNameMap: Map<OperationType, string> = new Map([
    [OperationType.Reset, 'resetDevice'],
    [OperationType.Activate, 'activateDevice'],
    [OperationType.ResetRoom, 'resetRoom'],
    [OperationType.ActivateRoom, 'activateRoom'],
]);

const operationDescriptionMap: Map<OperationType, string> = new Map([
    [OperationType.Reset, 'reset device'],
    [OperationType.Activate, 'activate device'],
    [OperationType.ResetRoom, 'reset room'],
    [OperationType.ActivateRoom, 'activate room'],
]);

const operationErrorCounterNameMap: Map<OperationType, string> = new Map([
    [OperationType.Reset, 'device__reset_error'],
    [OperationType.ResetRoom, 'room__reset_error'],
    [OperationType.Activate, 'device__activate_error'],
    [OperationType.ActivateRoom, 'room__activate_error'],
]);

export class OperationAlreadyRunning extends Error {
    public readonly existing: OperationInstance;

    constructor(existing: OperationInstance) {
        super("Operation is already running");
        this.existing = existing;
    }
}

const rejectOperationsBatch = async (operations: OperationInstance[], error: any, transaction: Transaction) => {
    await Promise.all(operations.map(operation => rejectOperation(operation, error, transaction)));
}

const operationUpdateHelper = (operation: OperationInstance) => ([cnt, operations]: [
    number,
    OperationInstance[],
]) => {
    if (cnt !== 1 || operations.length !== 1) {
        throw new Error('Operation was updated by another worker');
    }

    operation.set(operations[0].get(), {
        raw: true,
        reset: true,
    });

    return operation;
};

export const logOperationMessage = (logFunction: LogFunction, operation: OperationInstance, message: string, extraData?: any, error?: any) => {
    let prefix = operationDescriptionMap.get(operation.type);
    let opType = operationNameMap.get(operation.type)!;
    if (operation.scope?.context === 'customer') {
        prefix += ' for customer';
        opType += '4Customer';
    }
    logFunction(`${prefix}: ${message}`,
        {
            [opType]: {
                operation: operation.id,
                device: operation.devicePk || undefined,
                room: operation.roomPk || undefined,
                error,
                ...extraData
            }
        }
    )
}

export const updatePendingOperation = async (operation: OperationInstance, updatedSchema: OperationSchema, silent: boolean = false, transaction?: Transaction) => {
    await Operation.update(
        updatedSchema,
        {
            where: {
                id: operation.id,
                status: OperationStatus.Pending,
                updatedAt: operation.updatedAt as WhereValue
            },
            silent,
            transaction,
            returning: true
        }
    )
        .then(operationUpdateHelper(operation))
        .catch(async (updatePayloadError) => {
            let prefix = operationDescriptionMap.get(operation.type);
            let opType = operationNameMap.get(operation.type)!;
            if (operation.scope?.context === 'customer') {
                prefix += ' for customer';
                opType += '4Customer';
            }
            log.warn(`${prefix}: failed to update operation`, {
                [opType]: {
                    device: operation.devicePk,
                    operation: operation.id,
                },
            });

            const statusText = (updatedSchema.status) ? ` status=${updatedSchema.status}` : '';
            const apiError = new RestApiError(`Failed to update operation${statusText}`, 500, {
                origError: updatePayloadError,
            });

            await rejectOperation(operation, apiError);
            throw apiError;
        });
};

const updateOperationLastHandling = async (operation: OperationInstance, transaction?: Transaction) => {
    const now = new Date();
    return updatePendingOperation(operation, {
        lastHandling: now,
        updatedAt: now,
    } as OperationSchema, true, transaction);
}

export const updateOperationsLastHandling = async(operations: OperationInstance[]) => {
    return sequelize.transaction(async (transaction) =>
        Promise.all(operations.map(operation => updateOperationLastHandling(operation, transaction)))
    );
}

export const rejectOperation = async (operation: OperationInstance, error: any, transaction?: Transaction, rejectChildren: boolean = true) => {
    if (operation.children && operation.children.length > 0 && rejectChildren) {
        if (transaction) {
            throw new Error("Unexpected nested transaction");
        }
        await sequelize.transaction<void>(async (trans) => {
            await rejectOperationsBatch(operation.children!, error, trans);
        });
    }
    await Operation.update(
        {
            status: OperationStatus.Rejected,
            error: trimPrivateFields(serializeError(error)),
        } as OperationSchema,
        {
            where: {
                id: operation.id,
                status: OperationStatus.Pending,
                updatedAt: operation.updatedAt as WhereValue,
            },
            returning: true,
            transaction
        },
    )
        .then(operationUpdateHelper(operation))
        .then(() => {
            const counter = operationErrorCounterNameMap.get(operation.type);
            if (counter) {
                solomon.incCounter(`${counter}`);
            }
        })
        .catch((rejectOperationError) => {
            // если зарежектить не удалось, ничего страшного: придёт воркер и зарежектит по таймауту
            logOperationMessage(log.error, operation, 'failed to reject operation', undefined, rejectOperationError)
        })
};


export const createOperation = async (
    roomOrDevice: DeviceInstance | RoomInstance,
    scope: Scope,
    opType: OperationType,
    creationUpdateSchema?: DeviceSchema,
    deviceFilter: (d: DeviceInstance) => boolean = (d: DeviceInstance) => true
): Promise<OperationInstance> => {
    const room = (roomOrDevice instanceof RoomInstance) ? roomOrDevice as RoomInstance : undefined;
    if (room && room!.devices?.length === 0) {
        throw new Error("Illegal operation: attempt to run a batch operation on an empty room");
    }

    const batchType = room ? batchTypeMapping.get(opType) : undefined;
    const masterOpType = batchType || opType;
    let logPrefix = operationDescriptionMap.get(masterOpType);
    let opTypeName = operationNameMap.get(masterOpType);
    if (scope.context === 'customer') {
        logPrefix += ' for customer';
        opTypeName += '4Customer'
    }

    if (room && !batchType) {
        throw new Error(`Illegal operation: room-level operation type is not known for type ${opType}`);
    }

    if (opTypeName) {
        log.debug(`${logPrefix}: start`, {
            [opTypeName!]: {
                [roomOrDevice instanceof RoomInstance ? 'room' : 'device']: roomOrDevice.id,
            }
        })
    }

    const existingOpFilter: WhereOptions =
        room ? {
            type: batchType!,
            status: OperationStatus.Pending,
            roomPk: roomOrDevice.id
        } : {
            type: opType,
            status: OperationStatus.Pending,
            devicePk: roomOrDevice.id
        };

    const pendingOperation = await Operation.findOne({
        where: existingOpFilter
    });
    if (pendingOperation) {
        if (opTypeName) {
            log.debug(`${logPrefix}: found pending operation`, {
                [opTypeName!]: {
                    [roomOrDevice instanceof RoomInstance ? 'room' : 'device']: roomOrDevice.id,
                    operation: pendingOperation.id,
                }
            })
        }
        throw new OperationAlreadyRunning(pendingOperation);
    }
    const operationStart = new Date();
    const deviceIds = room ? room!.devices!.filter(deviceFilter).map(device => device.id) : [roomOrDevice.id];
    return await sequelize
        .transaction<OperationInstance>(async (transaction) => {
            if (creationUpdateSchema) {
                await Device.update(
                    creationUpdateSchema,
                    {
                        where: {
                            id: {
                                [Op.in]: deviceIds,
                            }
                        },
                        transaction,
                        returning: true
                    })
                    .then(([num]: [number, DeviceInstance[]]) => {
                        if (num !== deviceIds.length) {
                            throw new Error('Unexpected number of updated devices');
                        }
                    })
                    .catch((updateError) => {
                        if (opTypeName) {
                            log.debug(`${logPrefix}: failed to update initial device(s) state on operation start`, {
                                [opTypeName!]: {
                                    [roomOrDevice instanceof RoomInstance ? 'room' : 'device']: roomOrDevice.id,
                                },
                                error: updateError
                            });
                        }
                        throw updateError;
                    });
            }
            if (room) {
                return Operation.create(
                    {
                        type: batchType,
                        roomPk: room.id,
                        status: OperationStatus.Pending,
                        scope,

                        createdAt: operationStart,
                        updatedAt: operationStart,
                        lastHandling: operationStart,
                        children: deviceIds.map(deviceId => ({
                            type: opType,
                            devicePk: deviceId,
                            status: OperationStatus.Pending,
                            scope,

                            createdAt: operationStart,
                            updatedAt: operationStart,
                            lastHandling: operationStart,
                        })),
                    },
                    {
                        transaction,
                        include: [
                            {
                                model: Operation,
                                as: "children"
                            }
                        ],
                        silent: true, // использовать значение updatedAt из values
                    })
                    .catch((createOperationError) => {
                        if (opTypeName) {
                            log.debug(`${logPrefix}: failed to update initial devices state on operation start`, {
                                [opTypeName!]: {
                                    room: room.id,
                                },
                                error: createOperationError
                            });
                        }
                        throw isSequelizeUniqueConstraintError(createOperationError)
                            ? new RestApiError('Room is busy', 409, {
                                origError: createOperationError,
                            })
                            : createOperationError;
                    });
            } else {
                return Operation.create({
                    type: opType,
                    devicePk: roomOrDevice.id,
                    status: OperationStatus.Pending,
                    scope,

                    createdAt: operationStart,
                    updatedAt: operationStart,
                    lastHandling: operationStart,
                },
                    {
                        transaction,
                        silent: true, // использовать значение updatedAt из values
                    }
                ).catch((createOperationError) => {
                    if (opTypeName) {
                        log.debug(`${logPrefix}: failed to update initial device state on operation start`, {
                            [opTypeName!]: {
                                device: roomOrDevice.id,
                            },
                            error: createOperationError
                        });
                    }
                    throw isSequelizeUniqueConstraintError(createOperationError)
                        ? new RestApiError('Device is busy', 409, {
                            origError: createOperationError,
                        })
                        : createOperationError;
                });
            }
        })
        .catch(async (startOperationError) => {
            if (opTypeName) {
                log.warn(`${logPrefix}: failed to start operation`, {
                    [opTypeName!]: {
                        [roomOrDevice instanceof RoomInstance ? 'room' : 'device']: roomOrDevice.id,
                    },
                    error: startOperationError
                });
            }
            const rejectedOp = {
                type: masterOpType,
                devicePk: roomOrDevice instanceof DeviceInstance ? roomOrDevice.id : undefined,
                roomPk: roomOrDevice instanceof RoomInstance ? roomOrDevice.id : undefined,
                status: OperationStatus.Rejected,
                scope,
                error: trimPrivateFields(serializeError(startOperationError)),

                createdAt: operationStart,
                updatedAt: operationStart,
                lastHandling: operationStart,
            } as OperationSchema;
            await Operation.create(
                rejectedOp,
                {
                    silent: true
                })
                .catch(createRejectedOperationError => {
                    if (opTypeName) {
                        log.error(`${logPrefix}: failed to create rejected operation`, {
                            [opTypeName!]: {
                                [roomOrDevice instanceof RoomInstance ? 'room' : 'device']: roomOrDevice.id,
                            },
                            error: createRejectedOperationError
                        });
                    }
                    throw createRejectedOperationError;
                });
            const counter = operationErrorCounterNameMap.get(masterOpType);
            if (counter) {
                solomon.incCounter(`${counter}`);
            }
            throw RestApiError.fromError(startOperationError);
        });
};

import { Op } from 'sequelize';
import { Device, Operation } from '../../db';
import {
    OperationSchema,
    Status as OperationStatus,
    Type as OperationType,
} from '../../db/tables/operation';
import config from '../config';
import log from '../log';
import * as solomon from '../../services/solomon';
import { createAsyncWorker } from './utils';
import { serializeError } from 'serialize-error';
import { v4 as uuidv4 } from 'uuid';
import { trimPrivateFields } from '../errors/utils';
import { sendPush } from '../../services/push/send';

const routine = async () => {
    const logContext = {
        operationsWorkerUUID: uuidv4(),
    };
    log.debug(`operation worker: start`, logContext);

    const now = Date.now();
    const [, timeoutOperations] = await Operation.update(
        {
            status: OperationStatus.Rejected,
            error: trimPrivateFields(serializeError(new Error('Timeout'))),

            updatedAt: new Date(now),
        } as OperationSchema,
        {
            where: {
                status: OperationStatus.Pending,
                type: {
                    [Op.or]: [
                        OperationType.Reset,
                        OperationType.ResetRoom,
                        OperationType.Activate,
                        OperationType.ActivateRoom,
                        OperationType.PromoActivate,
                    ],
                },
                lastHandling: {
                    [Op.lte]: new Date(now - config.app.operationsWorker.rejectTimeout),
                },
            },
            returning: true,
        },
    );
    for (const operation of timeoutOperations) {
        log.warn(`operation worker: operation timed out`, {
            ...logContext,
            operation,
        });
        switch (operation.type) {
            case OperationType.Reset:
                solomon.incCounter('device__reset_error');
                break;
            case OperationType.Activate:
                solomon.incCounter('device__activate_error');
                break;
            case OperationType.PromoActivate:
                solomon.incCounter('promocode__activate_error');
                break;
        }
        const device = await Device.findOne({
            where: { id: operation.devicePk },
        });
        if (device) {
            sendPush({
                topic: device.organizationId,
                event: 'device-state',
                payload: device.id,
            });
        }
    }
    log.info(`operation worker: done`, logContext);
};

export const operationKillerWorker = createAsyncWorker({
    interval: config.app.operationsWorker.interval,
    routine,
    name: 'operation-killer',
});

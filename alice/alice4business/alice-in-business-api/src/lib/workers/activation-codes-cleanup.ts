import { Op } from 'sequelize';
import { ActivationCode } from '../../db';
import config from '../config';
import log from '../log';
import { createAsyncWorker } from './utils';
import { v4 as uuidv4 } from 'uuid';

const routine = async () => {
    const logContext = {
        activationCodesCleanupWorkerUUID: uuidv4(),
    };
    log.debug(`activation code cleanup: start`, logContext);

    const now = Date.now();
    const count = await ActivationCode.destroy({
        where: {
            createdAt: {
                [Op.lte]: new Date(now - config.app.customerActivationOperation.codeTTL),
            },
        },
    });

    log.info(`activation code cleanup: done, ${count} codes was destroyed`, {
        ...logContext,
        count,
    });
};

export const activationCodesCleanupWorker = createAsyncWorker({
    interval: config.app.activationCodesCleanupWorker.interval,
    routine,
    name: 'operation-killer',
});

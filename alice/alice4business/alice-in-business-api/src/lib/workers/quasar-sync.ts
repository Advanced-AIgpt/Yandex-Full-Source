import { Device } from '../../db';
import config from '../config';
import { createAsyncWorker } from './utils';
import { syncDeviceList } from '../sync/quasar';
import { Op } from 'sequelize';
import log from '../log';
import { v4 as uuidv4 } from 'uuid';

const routine = async () => {
    const logContext = {
        syncWorkerUUID: uuidv4(),
    };
    log.debug(`quasar sync: start`, logContext);

    const devices = await Device.findAll({
        where: {
            lastSyncStart: {
                [Op.lt]: new Date(Date.now() - config.quasar.retryInterval),
            },
        },
        limit: config.app.quasarSyncWorker.batchSize,
        order: [['lastSyncStart', 'ASC']],
    });
    log.debug(`quasar sync: got ${devices.length} devices`, {
        ...logContext,
        deviceIDs: devices.map((x) => x.id),
    });

    await syncDeviceList(devices, logContext);
    log.info(`syncWorker: finished for ${devices.length} devices`, logContext);
};

export const quasarSyncWorker = createAsyncWorker({
    interval: config.app.quasarSyncWorker.interval,
    routine,
    name: 'quasar-sync',
});

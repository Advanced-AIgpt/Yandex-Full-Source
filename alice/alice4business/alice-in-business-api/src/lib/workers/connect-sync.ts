import { createAsyncWorker } from './utils';
import { v4 as uuidv4 } from 'uuid';
import { Op } from 'sequelize';
import { ConnectOrganization } from '../../db';
import config from '../config';
import log from '../log';
import { syncOrganization } from '../sync/connect';

const routine = async () => {
    const logContext = {
        connectSyncWorkerUUID: uuidv4(),
    };
    log.debug(`connect sync: start`, logContext);

    const outdatedConnectOrganizations = await ConnectOrganization.findAll({
        where: {
            active: true,
            lastSync: {
                [Op.or]: [
                    null,
                    {
                        [Op.lte]: new Date(
                            Date.now() - config.app.connectSyncWorker.minAgeBeforeSync,
                        ),
                    },
                ],
            },
            updatedAt: {
                [Op.lte]: new Date(
                    Date.now() - config.app.connectSyncWorker.rejectTimeout,
                ),
            },
        },
        order: [['lastSync', 'ASC', 'NULLS FIRST']],
        limit: config.app.connectSyncWorker.batchSize,
    });
    log.debug(
        `connect sync: got ${outdatedConnectOrganizations.length} Connect organizations`,
        {
            ...logContext,
            connectOrgIds: outdatedConnectOrganizations.map((x) => x.id),
        },
    );

    await Promise.all(
        outdatedConnectOrganizations.map((item) =>
            syncOrganization(item.id, logContext).catch((error) => {
                log.warn('connect sync: failed', {
                    ...logContext,
                    connectOrgId: item.id,
                    error,
                });
            }),
        ),
    );
    log.info(`connect sync: done`, logContext);
};

export const connectSyncWorker = createAsyncWorker({
    interval: config.app.connectSyncWorker.interval,
    routine,
    name: 'connect-sync',
});

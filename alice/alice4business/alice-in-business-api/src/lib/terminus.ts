import { createTerminus } from '@godaddy/terminus';
import { Server } from 'http';
import { promisify } from 'util';
import config from './config';
import log from './log';

const setTimeoutPromise = promisify(setTimeout);

export default function configureTerminus(server: Server) {
    createTerminus(server, {
        healthChecks: {
            [config.terminus.healthCheckRoute]: async () => {
                log.debug('terminus:healthcheck');
            },
        },
        timeout: config.terminus.timeout,
        signal: config.terminus.signal,
        beforeShutdown: async () => {
            log.debug('terminus:beforeShutdown:start');

            await setTimeoutPromise(config.terminus.beforeShutdownTimeout);

            log.debug('terminus:beforeShutdown:end');
        },
        onShutdown: async () => {
            log.debug('terminus: onShutdown');

            if (config.terminus.exitOnShutdown) {
                process.exit();
            }
        },
        logger: (msg, err) => {
            return err ? log.error(err, { msg }) : log.info('terminus:' + msg);
        },
    });
}

import express from 'express';
import morgan from 'morgan';
import config from './lib/config';
import log from './lib/log';
import httpContext from 'express-http-context';
import router from './router';
import { activationCodesCleanupWorker } from './lib/workers/activation-codes-cleanup';
import { operationKillerWorker } from './lib/workers/operation-killer';
import { promoCounterWorker } from './lib/workers/promo-counter';
import { quasarSyncWorker } from './lib/workers/quasar-sync';
import { connectSyncWorker } from './lib/workers/connect-sync';
import * as solomon from './services/solomon';
import { IncomingHttpHeaders } from 'http';
import expressXForwardedForFix = require('@yandex-int/express-x-forwarded-for-fix');

interface MakeAppParams {
    enableActivationCodesCleanupWorker?: boolean;
    enableOperationKillerWorker?: boolean;
    enablePromoCounterWorker?: boolean;
    enableQuasarSyncWorker?: boolean;
    enableConnectSyncWorker?: boolean;
}

export default function makeApp({
    enableActivationCodesCleanupWorker = true,
    enableOperationKillerWorker = true,
    enablePromoCounterWorker = true,
    enableQuasarSyncWorker = true,
    enableConnectSyncWorker = true,
}: MakeAppParams = {}) {
    const [
        startActivationCodesCleanupWorker,
        stopActivationCodesCleanupWorker,
    ] = activationCodesCleanupWorker;
    const [startOperationKillerWorker, stopOperationKillerWorker] = operationKillerWorker;
    const [startPromoCounterWorker, stopPromoCounterWorker] = promoCounterWorker;
    const [startQuasarSyncWorker, stopQuasarSyncWorker] = quasarSyncWorker;
    const [startConnectSyncWorker, stopConnectSyncWorker] = connectSyncWorker;

    if (
        config.app.activationCodesCleanupWorker.enable &&
        enableActivationCodesCleanupWorker
    ) {
        startActivationCodesCleanupWorker();
    }
    if (config.app.operationsWorker.enable && enableOperationKillerWorker) {
        startOperationKillerWorker();
    }
    if (config.app.countersWorker.enable && enablePromoCounterWorker) {
        startPromoCounterWorker();
    }
    if (config.app.quasarSyncWorker.enable && enableQuasarSyncWorker) {
        startQuasarSyncWorker();
    }
    if (config.app.connectSyncWorker.enable && enableConnectSyncWorker) {
        startConnectSyncWorker();
    }
    process.on('SIGUSR2', () => {
        stopActivationCodesCleanupWorker();
        stopOperationKillerWorker();
        stopPromoCounterWorker();
        stopQuasarSyncWorker();
        stopConnectSyncWorker();
    });

    if (config.app.debug.stackTraceSourceMapSupport) {
        require('source-map-support/register'); // tslint:disable-line:no-var-requires
    }

    if (config.app.debug.cutInternalStackEntries) {
        require('clarify'); // tslint:disable-line:no-var-requires
    }

    const app = express();
    app.set('env', config.environment);
    app.disable('x-powered-by');
    app.enable('trust proxy');
    app.use(expressXForwardedForFix());

    if (process.env.PASKILLS_LOGGER_ENABLED !== 'false') {
        app.use(morgan('combined'));
    }

    app.use(solomon.middleware);

    app.use(httpContext.middleware as express.RequestHandler);

    app.use((req, res, next) => {
        const { headers = {}, originalUrl } = req;

        httpContext.set('requestMeta', {
            id: headers['x-request-id'],
            url: originalUrl,
        });

        next();
    });

    app.use((req, res, next) => {
        const headers: IncomingHttpHeaders = {};
        for (const name of Object.keys(req.headers).sort()) {
            switch (name) {
                case 'x-ya-user-ticket':
                case 'x-ya-service-ticket': {
                    const parts = (req.headers[name] as string).split(':');
                    parts[parts.length - 1] = '***';
                    headers[name] = parts.join(':');

                    break;
                }

                case 'authorization': {
                    const parts = req.headers.authorization!.split(' ', 2);
                    if (parts.length > 1) {
                        parts[1] = '***';
                    }
                    headers[name] = parts.join(' ');
                    break;
                }

                case 'cookie':
                    headers[name] = '***';
                    break;

                default:
                    headers[name] = req.headers[name];
            }
        }
        log.debug('Request headers:', { headers });
        next();
    });

    if (!config.terminus.enabled) {
        app.get('/ping', (req, res) => {
            if (app.get('stopped')) {
                res.sendStatus(500);
            } else {
                res.send('pong');
            }
        });

        process.on('SIGUSR2', () => {
            app.set('stopped', true);
        });
    }
    solomon.startSolomon()

    app.use(config.app.urlRoot, router);

    return app;
}

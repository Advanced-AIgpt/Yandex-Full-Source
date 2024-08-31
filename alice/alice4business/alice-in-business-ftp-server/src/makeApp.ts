import express from 'express';
import httpContext from 'express-http-context';
import { IncomingHttpHeaders } from 'http';
import expressXForwardedForFix from '@yandex-int/express-x-forwarded-for-fix';

import log from './lib/log';

export default function makeApp() {
    const app = express();
    app.set('env', 'production');
    app.disable('x-powered-by');
    app.enable('trust proxy');
    app.use(expressXForwardedForFix());

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


    return app;
}

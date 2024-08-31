import cookieParser from 'cookie-parser';
import express from 'express';
import expressXForwardedForFix from '@yandex-int/express-x-forwarded-for-fix';
import expressBlackbox from '@yandex-int/express-blackbox';
import expressDomainAccess from '@yandex-int/express-domain-access';
import expressTld from '@yandex-int/express-tld';
import csp from '@yandex-int/express-yandex-csp';
import expressYandexuid from '@yandex-int/express-yandexuid';
import expressSecretkey from '@yandex-int/express-secretkey';
import expressHttpUatraitsMiddleware from '@yandex-int/express-http-uatraits';
import path from 'path';
import React from 'react';
import RDS from 'react-dom/server';
import config from './lib/config';
import helmet from 'helmet';
import { getServiceTicketForBlackbox } from './lib/tvm';
import apiRouter from './routers/api';
import publicApiRouter from './routers/api-public';
import supportApiRouter from './routers/api-support';
import authenticate from './middlewares/authenticate';

import IndexViewType from './views/Index';
import CustomerViewType from './views/Customer';
import StationViewType from './views/Station';
import SupportViewType from './views/Support';
// tslint:disable-next-line:no-var-requires
const IndexView: typeof IndexViewType = require('../../build/server/views/Index.bundle');
// tslint:disable-next-line:no-var-requires
const CustomerView: typeof CustomerViewType = require('../../build/server/views/Customer.bundle');
// tslint:disable-next-line:no-var-requires
const StationView: typeof StationViewType = require('../../build/server/views/Station.bundle');
// tslint:disable-next-line:no-var-requires
const SupportView: typeof SupportViewType = require('../../build/server/views/Support.bundle');

const assetsPath = path.resolve(__dirname, '../../build/assets');
const getRoute = (route: string) => path.join(config.app.urlRoot, route);

const app = express();

app.disable('x-powered-by');
app.enable('trust proxy');
app.use(expressXForwardedForFix());

app.use(
    helmet({
        dnsPrefetchControl: false,
    }),
);

// fast routes without middlewares
app.get('/ping', (req, res) => {
    if (app.get('stopped')) {
        res.sendStatus(500);
    } else {
        res.send('pong');
    }
});

app.use(getRoute('/assets'), express.static(assetsPath));

// common middlewares
app.use(expressTld());

// public api
app.use(config.app.apiPublicProxy, publicApiRouter);

// cookie-based authentication
app.use(expressDomainAccess('ru'));
app.use(cookieParser());
app.use(
    expressBlackbox({
        ...config.blackbox,
        getServiceTicket: getServiceTicketForBlackbox,
    }),
);
app.use(expressHttpUatraitsMiddleware());
app.use(expressYandexuid());
app.use(expressSecretkey());

// support api
app.use(config.app.apiSupportProxy, supportApiRouter);

// internal api
app.use(
    config.app.apiProxy,
    express
        .Router()
        .use('/customer/guest', authenticate({ onError: 'ignore' }), apiRouter)
        .use(authenticate({ onError: 'error' }), apiRouter),
);

// browser
app.get(
    getRoute('/customer'),
    csp({
        policies: {
            'default-src': [csp.NONE],
            'frame-src': ['https://yandex.ru'],
            'font-src': ["'self'", 'yastatic.net'],
            'script-src': ["'self'", "'unsafe-eval'", '%nonce%', 'mc.yandex.ru', 'yastatic.net'],
            'style-src': ["'self'", "'unsafe-inline'", 'mc.yandex.ru', 'yastatic.net'],
            'img-src': [
                "'self'",
                'data:',
                config.avatar.host,
                'mc.yandex.ru',
                'yastatic.net',
                'mc.admetrica.ru',
            ],
            'frame-ancestors': ['webvisor.com', 'http://webvisor.com'],
            'media-src': ["'self'", 'yastatic.net'],
            'connect-src': ["'self'", 'mc.yandex.ru', config.passportAccounts.host],
        },
    }),
    (req, res) => {
        res.send('<!DOCTYPE html>' + RDS.renderToString(<CustomerView req={req} />));
    },
);

app.get(
    getRoute('/station'),
    csp({
        policies: {
            'default-src': [csp.NONE],
            'frame-src': ['https://yandex.ru'],
            'font-src': ["'self'", 'yastatic.net'],
            'script-src': ["'self'", "'unsafe-eval'", '%nonce%', 'mc.yandex.ru', 'yastatic.net'],
            'style-src': ["'self'", "'unsafe-inline'", 'mc.yandex.ru', 'yastatic.net'],
            'img-src': ["'self'", 'data:', 'mc.yandex.ru', 'yastatic.net', 'mc.admetrica.ru'],
            'frame-ancestors': ['webvisor.com', 'http://webvisor.com'],
            'media-src': ["'self'", 'yastatic.net'],
            'connect-src': ["'self'", 'mc.yandex.ru'],
        },
    }),
    (req, res) => {
        res.send(
            '<!DOCTYPE html>' +
                RDS.renderToString(
                    <StationView
                        host={req.hostname}
                        activationCode={
                            (Array.isArray(req.query.code) ? req.query.code[0] : req.query.code) ||
                            ''
                        }
                    />,
                ),
        );
    },
);

app.get(
    getRoute('/support'),
    authenticate({ onError: 'redirect' }),
    csp({
        policies: {
            'default-src': [csp.NONE],
            'frame-src': ['https://yandex.ru'],
            'font-src': ["'self'", 'yastatic.net'],
            'script-src': [
                "'self'",
                "'unsafe-eval'",
                "'unsafe-inline'",
                '%nonce%',
                'mc.yandex.ru',
                'yastatic.net',
                'chat.s3.yandex.net',
            ],
            'style-src': ["'self'", "'unsafe-inline'", 'mc.yandex.ru', 'yastatic.net'],
            'img-src': [
                "'self'",
                'data:',
                config.avatar.host,
                'mc.yandex.ru',
                'yastatic.net',
                'mc.admetrica.ru',
            ],
            'frame-ancestors': ['webvisor.com', 'http://webvisor.com'],
            'media-src': ["'self'", 'yastatic.net'],
            'connect-src': ["'self'", 'mc.yandex.ru', config.push.host],
        },
    }),
    (req, res) => {
        res.send(
            '<!DOCTYPE html>' +
            RDS.renderToStaticMarkup(
                <SupportView
                    req={req}
                />,
            ),
        );
    },
);

app.get(
    '/*',
    authenticate({ onError: 'redirect' }),
    csp({
        policies: {
            'default-src': [csp.NONE],
            'frame-src': ['https://yandex.ru'],
            'font-src': ["'self'", 'yastatic.net'],
            'script-src': [
                "'self'",
                "'unsafe-eval'",
                "'unsafe-inline'",
                '%nonce%',
                'mc.yandex.ru',
                'yastatic.net',
                'chat.s3.yandex.net',
            ],
            'style-src': ["'self'", "'unsafe-inline'", 'mc.yandex.ru', 'yastatic.net'],
            'img-src': [
                "'self'",
                'data:',
                config.avatar.host,
                'mc.yandex.ru',
                'yastatic.net',
                'mc.admetrica.ru',
            ],
            'frame-ancestors': ['webvisor.com', 'http://webvisor.com'],
            'media-src': ["'self'", 'yastatic.net'],
            'connect-src': ["'self'", 'mc.yandex.ru', config.push.host],
        },
    }),
    (req, res) => {
        res.send(
            '<!DOCTYPE html>' +
                RDS.renderToStaticMarkup(
                    <IndexView
                        blackbox={req.blackbox!}
                        yu={req.cookies.yandexuid}
                        secretkey={req.secretkey!}
                    />,
                ),
        );
    },
);

export default app;

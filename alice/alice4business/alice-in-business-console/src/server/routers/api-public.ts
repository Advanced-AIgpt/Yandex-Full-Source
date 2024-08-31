import cookieParser from 'cookie-parser';
import express from 'express';
import expressBlackbox from '@yandex-int/express-blackbox';
import expressDomainAccess from '@yandex-int/express-domain-access';
import proxy from 'http-proxy-middleware';
import log from '../lib/log';
import { getServiceTicketForBlackbox } from '../lib/tvm';
import config from '../lib/config';

const router = express.Router();

router.use(expressDomainAccess('net'));
router.use(cookieParser());
router.use((req, res, next) => {
    console.log(req.headers.authorization);
    next();
});

router.use(
    expressBlackbox({
        ...config.blackbox,
        getServiceTicket: getServiceTicketForBlackbox,
        oauth: true,
    }),
);

router.use((req, res, next) => {
    if (!req.blackbox) {
        return res.sendStatus(403);
    }

    if (req.blackbox.error === 'expired_token') {
        log.debug('checkOAuthToken error (TOKEN_EXPIRED)', req.blackbox);
        return res.sendStatus(403);
    }

    if (req.blackbox.error !== 'OK') {
        log.debug('checkOAuthToken error (maybe TVM)', req.blackbox);
        return res.sendStatus(403);
    }

    const oauth = (req.blackbox.raw as ExpressBlackbox.PassportOauthResponse).oauth;
    if (oauth.client_id !== config.app.oauthClientId) {
        log.debug('checkOAuthToken error (client id mismatch)', oauth);
        return res.sendStatus(403);
    }
    next();
});

router.use(
    proxy({
        target: config.api.url,
        pathRewrite: (path, req) => {
            return path.replace(config.app.apiProxy, '');
        },
        changeOrigin: true,
        secure: false,
        logLevel: 'error',
        proxyTimeout: config.app.apiTimeout,
        logProvider: () => log,
        onProxyReq(proxyReq, req: express.Request) {
            proxyReq.setHeader('X-Forwarded-For-Y', req.ip);
            if (req.blackbox && req.blackbox.userTicket) {
                proxyReq.setHeader('x-ya-user-ticket', req.blackbox.userTicket);
                proxyReq.setHeader('authorization', req.headers.authorization!);
            }
        },
    }),
);
export default router;

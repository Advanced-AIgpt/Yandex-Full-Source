import express from 'express';
import proxy from 'http-proxy-middleware';
import log from '../lib/log';
import config from '../lib/config';
import { getServiceTicketForApi } from '../lib/tvm';

interface CustomRequest extends express.Request {
    selfTicket: string;
}

const router = express.Router();

router.use((_req, res, next) => {
    const req = _req as CustomRequest;

    getServiceTicketForApi()
        .then((ticket) => {
            req.selfTicket = ticket;
            next();
        })
        .catch((error) => {
            log.warn('Failed to get service ticket', { error });
            return res.sendStatus(403);
        });
});

router.use(
    proxy({
        target: config.api.url,
        pathRewrite: (path, req) => {
            return path.replace(config.app.apiProxy, '');
        },
        changeOrigin: true,
        proxyTimeout: config.app.apiTimeout,
        secure: false,
        logLevel: 'info',
        logProvider: () => log,
        onProxyReq(proxyReq, req: CustomRequest) {
            proxyReq.removeHeader('Authorization');
            // proxyReq.removeHeader('Cookie'); // Cookie is required to register kolonkish
            proxyReq.setHeader('X-Ya-Service-Ticket', req.selfTicket);
            if (req.blackbox && req.blackbox.userTicket) {
                proxyReq.setHeader('X-Ya-User-Ticket', req.blackbox.userTicket);
            } else {
                proxyReq.removeHeader('X-Ya-User-Ticket');
            }
            if (req.blackbox && req.blackbox.connect) {
                proxyReq.setHeader('X-Connect-Org-Id', req.blackbox.connect);
            } else {
                proxyReq.removeHeader('X-Connect-Org-Id');
            }

            proxyReq.setHeader('User-Agent', 'alice4business-console/1.0');

            for (const header of proxyReq.getHeaderNames()) {
                if (header.toLowerCase().startsWith('x-forwarded-for')) {
                    proxyReq.removeHeader(header);
                }
            }
            proxyReq.setHeader('X-Forwarded-For-Y', req.ip);

            log.info('API request', req.originalUrl);
        },
    }),
);

router.use(((err, req, res, next) => {
    if (err.code === 'LIMIT_FILE_SIZE') {
        return res.status(413).json({
            error: {
                name: err.code,
                code: 413,
                message: err.message,
            },
        });
    }

    if (err.code === 'UNSUPPORTED_FILE_TYPE') {
        return res.status(400).json({
            error: {
                name: err.code,
                code: 400,
                message: err.message,
            },
        });
    }

    if (err.code === 'EBADCSRFTOKEN') {
        return res.status(403).json({
            error: {
                name: err.code,
                code: 403,
                message: err.message,
            },
        });
    }

    log.error(err);

    return res.status(500).json({
        error: {
            name: 'Internal server error',
            code: 500,
            message: 'Internal server error',
        },
    });
}) as express.ErrorRequestHandler);

export default router;

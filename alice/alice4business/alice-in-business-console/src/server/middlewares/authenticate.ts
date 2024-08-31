import express from 'express';
import MissingMiddlewareError from 'missing-middleware-error';
import url from 'url';
import log from '../lib/log';
import config from '../lib/config';

function extractHost(req: express.Request) {
    const { host } = req.headers;

    return Array.isArray(host) ? host[0] : host;
}

function getPassportRedirectUrl(req: express.Request) {
    const parsedUrl = url.parse(req.originalUrl);
    return url.format({
        protocol: config.passport.protocol,
        host: config.passport.host,
        pathname: '/auth',
        query: {
            retpath: url.format({
                protocol: req.protocol,
                host: extractHost(req),
                pathname: parsedUrl.pathname,
                query: parsedUrl.query,
                search: parsedUrl.search,
            }),
        },
    });
}

interface Options {
    onError: 'redirect' | 'error' | 'ignore';
}

export default ({ onError }: Options): express.RequestHandler => (req, res, next) => {
    if (!req.blackbox) {
        return next(new MissingMiddlewareError('express-blackbox'));
    }
    if (!req.blackbox.uid) {
        log.debug(`Blackbox error: ${req.blackbox.error}`, req.blackbox);

        switch (onError) {
            case 'redirect':
                res.redirect(getPassportRedirectUrl(req));
                return;
            case 'error':
                res.sendStatus(403);
                return;
            case 'ignore':
        }
    }

    next();
};

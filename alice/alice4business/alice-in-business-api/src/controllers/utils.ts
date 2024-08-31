import { ErrorRequestHandler, Request, RequestHandler, Response } from 'express';
import RestApiError from '../lib/errors/restApi';
import { isEmptyResultError } from '../lib/errors/utils';
import { is5xxErr } from '../services/got/helpers';
import log from '../lib/log';

type AsyncRequestHandler = (req: Request, res: Response) => Promise<any>;
type RequestHandlerWrapper<P = {}> = (
    fn: AsyncRequestHandler,
    params?: P,
) => RequestHandler;

export const asyncJsonResponse: RequestHandlerWrapper<{
    wrapResult?: boolean;
    putPostStatus?: number;
}> = (fn, params = {}) => (_req, res, next) => {
    const req = _req as Request;
    const { wrapResult = false, putPostStatus = 201 } = params;

    if (['POST', 'PUT'].includes(req.method)) {
        res.status(putPostStatus);
    }

    fn(req, res)
        .then((result) => {
            if (wrapResult) {
                res.json({ result });
            } else {
                res.json(result);
            }
        })
        .catch(next);
};

export const asyncMiddleware: RequestHandlerWrapper = (fn) => (req, res, next) => {
    fn(req as Request, res)
        .then(next)
        .catch(next);
};

export const errorHandler: (params?: {
    mode?: 'wrap' | 'wrap+payload' | 'payload';
}) => ErrorRequestHandler = (params = {}) => (err, req, res, next) => {
    const restApiError = RestApiError.fromError(err);
    const { fields, message, statusCode, origError, silent, payload } = restApiError;

    if (!silent) {
        const logLevel = is5xxErr(statusCode) ? 'error' : 'info';
        log[logLevel](restApiError, { origError });
    }

    res.status(statusCode);
    switch (params.mode) {
        case 'wrap':
            res.json({
                status: 'error',
                error: {
                    message,
                    code: statusCode,
                    ...(fields && { fields }),
                },
            });
            break;
        case 'wrap+payload':
            res.json({
                status: 'error',
                error: {
                    message,
                    code: statusCode,
                    ...(fields && { fields }),
                },
                payload,
            });
            break;
        case 'payload':
            res.json(payload);
            break;
        default:
            res.json({ message, payload });
    }
};

export const notFound = (name: string) => (error: any) => {
    if (isEmptyResultError(error)) {
        throw new RestApiError(`${name} not found`, 404);
    }
    throw error;
};

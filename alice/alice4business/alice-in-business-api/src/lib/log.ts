import kroniko from 'kroniko';
import httpContext from 'express-http-context';
import { OperationInstance } from '../db/tables/operation';

if (process.env.PASKILLS_LOGGER_ENABLED !== 'false') {
    kroniko.install({
        breadcrumbs: {
            http: true,
            trace: true,
        },
        std: {
            format: 'qloud',
            depth: Infinity,
            pretty: true,
            time: true,
        },
    });
}

const buildOpMessage = (message: any, operation?: OperationInstance) => {
    if (typeof message !== 'string' || !operation) {
        return message;
    }
    return `${message}: ${operation.type} ID --> ${operation.id}`;
};

interface LogPayload {
    operation?: OperationInstance;
    [key: string]: any;
}

const wrap = (fn: kroniko.LogFunction) => (...args: [any, LogPayload?]) => {
    const operation = args[1] ? args[1].operation : undefined;
    return fn(buildOpMessage(args[0], operation), {
        ...args[1],
        request: httpContext.get('requestMeta'),
        operation: operation ? operation.toJSON() : operation,
    });
};

const log = {
    ...kroniko,
    log: wrap(kroniko.log),
    warn: wrap(kroniko.warn),
    error: wrap(kroniko.error),
    debug: wrap(kroniko.debug),
    info: wrap(kroniko.info),
    trace: wrap(kroniko.trace),
};

export default log;

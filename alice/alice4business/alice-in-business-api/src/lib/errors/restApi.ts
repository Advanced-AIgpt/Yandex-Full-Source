import {
    isEmptyResultError,
    isMulterError,
    isSequelizeUniqueConstraintError,
    isSequelizeValidationError,
} from './utils';

interface RestApiErrorParams {
    origError?: Error;
    fields?: Record<string, string | string[]>;
    silent?: boolean;
    payload?: ErrorPayload;
    invalid?: boolean;
    userMessage?: string;
}

class MalformedError extends Error {
    constructor(message: unknown) {
        super(JSON.stringify(message));
    }
}

export default class RestApiError extends Error {
    public static fromError(error: unknown) {
        if (!(error instanceof Error)) {
            return new RestApiError('Unexpected error', 500, {
                origError: new MalformedError(error),
            });
        }

        if (error instanceof RestApiError) {
            return error;
        }

        if (isMulterError(error)) {
            return new RestApiError('Bad request', 400, { origError: error });
        }

        if (isEmptyResultError(error)) {
            return new RestApiError('Resource not found', 404, { origError: error });
        }

        if (isSequelizeUniqueConstraintError(error)) {
            return new RestApiError('Unique constraint error', 400, {
                fields: (error as any).fields,
                origError: error,
                payload: { unique: true },
            });
        }

        if (isSequelizeValidationError(error)) {
            throw new RestApiError('Validation error', 400, {
                fields: (error as any).fields,
                origError: error,
                payload: { invalid: true },
            });
        }

        return new RestApiError('Unexpected error', 500, { origError: error });
    }

    public readonly statusCode: number;
    public readonly origError?: Error;
    public readonly silent: boolean;
    public readonly fields?: RestApiErrorParams['fields'];
    public readonly payload?: ErrorPayload;

    constructor(message: string, statusCode: number, params: RestApiErrorParams = {}) {
        super(message);
        this.name = this.constructor.name;
        this.statusCode = statusCode;
        this.fields = params.fields;
        this.silent = Boolean(params.silent);
        this.payload = params.payload;

        if (params.origError) {
            this.origError = params.origError;
            this.stack = `${this.stack}\n${params.origError.stack}`;
        }
    }
}

/**
 * @property errorCode нужно для кастомных кодов ошибок. Например, когда не достаточно HTTP кодов
 */
type ErrorPayload = Record<string, any> & {
    errorCode?: string;
};

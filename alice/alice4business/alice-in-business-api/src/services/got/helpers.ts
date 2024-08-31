import { CounterType } from './types';
import { GotError } from 'got';

export const isConnectionError = (error: GotError): error is GotError => {
    return error.code?.startsWith('ECONN') ?? false;
};

export const is4xxErr = (statusCode: number) => {
    return statusCode >= 400 && statusCode < 500;
};

export const is5xxErr = (statusCode: number) => {
    return statusCode >= 500;
};

export const getHttpStatusType = (statusCode: number) => {
    const leadingDigit = String(statusCode)[0];

    return `${leadingDigit}xx` as CounterType;
};

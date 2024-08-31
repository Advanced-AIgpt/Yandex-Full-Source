import request from 'supertest';
import { Assertions } from 'ava';

export type HTTPMethod = 'get' | 'post' | 'put' | 'delete' | 'patch';

export interface CallRestApiOptions {
    userTicket?: string;
    serviceTicket?: string;
    contentType?: string;
}

export type CallRestApi = (
    method: HTTPMethod,
    route: string,
    options?: CallRestApiOptions,
) => Promise<request.Test>;

export interface ApiResponds {
    code?: number;
    wrapResult?: boolean;
    t: Assertions;
    res: request.Response;
    expect: any;
}
export interface ApiErrorResponds {
    code: number;
    message?: string;
    res: request.Response;
    t: Assertions;
}

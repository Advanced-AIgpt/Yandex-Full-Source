import { AnonymousUserScope, UserScope } from '../db/tables/operation';
import { Request } from 'express';

export const sleep = (ms: number) => new Promise((resolve) => setTimeout(resolve, ms));

export const serializeOperationScope = async (
    req: Request,
    context: UserScope['context'],
): Promise<UserScope> => ({
    url: req.originalUrl,
    context,
    body: req.body,
    method: req.method,
    params: req.params,
    userId: req.user.uid.toString(10),
    userLogin: await req.user.login,
});

export const serializeAnonimousOperationScope = (req: Request): AnonymousUserScope => {
    return {
        url: req.originalUrl,
        context: 'customer',
        body: req.body,
        method: req.method,
        params: req.params,
        userId: null,
        userLogin: null,
    };
};

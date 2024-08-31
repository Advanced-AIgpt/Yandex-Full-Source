import request from 'supertest';
import makeApp from '../../makeApp';
import { getOauth } from '../../services/blackbox';
import data from './data';
import { bindUserToOrganization, createOrganization, createUser } from './db';
import { ApiErrorResponds, ApiResponds, HTTPMethod } from './types';
import { getServiceTickets } from '../../services/tvm';
import config from '../../lib/config';
import ip from 'ip';
import path from 'path';
import * as Authentication from '../../lib/middlewares/authentication';
import sinon from 'sinon';

export const getUserTicket = async (oauthToken: string) => {
    const response = await getOauth({
        userip: ip.address(),
        oauth_token: oauthToken,
        get_user_ticket: 'yes',
    });
    return response.body.user_ticket;
};

// We have to redefine tvmServiceTicketChecker's argument value for tests
const _tvmServiceTicketCheckerOrig = Authentication.tvmServiceTicketChecker;
const _tvmServiceTicketCheckerStub = sinon
    .stub(Authentication, 'tvmServiceTicketChecker')
    .callsFake((permittedTVMAppIds: Set<number> | null) => {
        if (permittedTVMAppIds === null) {
            return _tvmServiceTicketCheckerOrig(null);
        } else {
            return _tvmServiceTicketCheckerOrig(
                new Set<number>([config.tvmtool.selfId]),
            );
        }
    });

const appInstance = makeApp({
    enableActivationCodesCleanupWorker: false,
    enablePromoCounterWorker: false,
    enableOperationKillerWorker: false,
    enableQuasarSyncWorker: false,
    enableConnectSyncWorker: false,
});
_tvmServiceTicketCheckerStub.restore();

const makeRestApiCaller = (
    prefix: string,
    method: HTTPMethod,
    route: string,
    options?: { body?: object; query?: object | string },
) => {
    const res = request(appInstance)[method](
        path.posix.join(config.app.urlRoot, prefix, route),
    );

    if (options?.query) {
        res.query(options.query);
    }

    if (options?.body) {
        res.send(options.body);
    }

    res.set('x-forwarded-for-y', data.user.ip);

    return res;
};

export const callInternalDialogovoApi = async (
    method: HTTPMethod,
    route: string,
    options?: { body?: object; query?: object | string },
) =>
    makeRestApiCaller('internal/dialogovo', method, route, options).set(
        'x-ya-service-ticket',
        (await getServiceTickets([config.tvmtool.selfAlias]))[config.tvmtool.selfAlias]
            .ticket,
    );

export const callInternalQuasarApi = async (
    method: HTTPMethod,
    route: string,
    options?: { body?: object; query?: object | string },
) => makeRestApiCaller('internal/quasar', method, route, options);

export const callPublicApi = async (
    method: HTTPMethod,
    route: string,
    options?: { body?: object; query?: object | string },
) =>
    makeRestApiCaller('public', method, route, options)
        .set(
            'x-ya-service-ticket',
            (await getServiceTickets([config.tvmtool.selfAlias]))[
                config.tvmtool.selfAlias
            ].ticket,
        )
        .set('x-ya-user-ticket', await getUserTicket(data.userOAuthToken));

export const callConsoleApi = async (
    method: HTTPMethod,
    route: string,
    options?: { body?: object; query?: object | string },
) =>
    makeRestApiCaller('console', method, route, options)
        .set(
            'x-ya-service-ticket',
            (await getServiceTickets([config.tvmtool.selfAlias]))[
                config.tvmtool.selfAlias
            ].ticket,
        )
        .set('x-ya-user-ticket', await getUserTicket(data.userOAuthToken));

export const createCredentials = async () => {
    await createUser();
    await createOrganization();
    await bindUserToOrganization();
};

export const responds = ({
    expect,
    code = 200,
    res,
    wrapResult = true,
    t,
}: ApiResponds) => {
    expect = wrapResult ? { result: expect } : expect;
    t.is(res.status, code);
    t.deepEqual(res.body, expect);
};

export const respondsWithError = ({ code, message, t, res }: ApiErrorResponds) => {
    t.is(res.status, code);
    const { error } = res.body;
    t.deepEqual(
        {
            message: message ? error.message : undefined,
            code: error.code,
        },
        {
            message,
            code,
        },
    );
};

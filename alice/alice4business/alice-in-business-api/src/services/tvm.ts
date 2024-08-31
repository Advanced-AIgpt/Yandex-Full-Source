import config from '../lib/config';
import createGot from './got';
import { Request } from 'express';

const tvmGot = createGot({
    sourceName: 'tvm',
    baseUrl: config.tvmtool.host,
});

export const getServiceTickets = async (dsts: string[], src?: string) =>
    tvmGot(`/tvm/tickets?src=${src || config.tvmtool.selfAlias}&dsts=${dsts.join(',')}`, {
        headers: {
            Authorization: config.tvmtool.token,
        },
        json: true,
    }).then((r) => r.body);

interface CheckServiceResult {
    src: number;
    dst: number;
    scopes: string[] | null;
}
export const checkServiceTicket = async (
    serviceTicket: string,
    dst?: string,
): Promise<CheckServiceResult> =>
    tvmGot(`/tvm/checksrv?dst=${dst || config.tvmtool.selfAlias}`, {
        headers: {
            authorization: config.tvmtool.token,
            'x-ya-service-ticket': serviceTicket,
        },
        json: true,
    }).then((r) => r.body);

interface CheckUserResult {
    default_uid: number;
    uids: number[];
    scopes: string[] | null;
}
export const checkUserTicket = async (userTicket: string): Promise<CheckUserResult> =>
    tvmGot('/tvm/checkusr', {
        headers: {
            authorization: config.tvmtool.token,
            'x-ya-user-ticket': userTicket,
        },
        json: true,
    }).then((r) => r.body);

export const getTicketsFor = async (serviceName: string, req: Request) => {
    try {
        const serviceTickets = await getServiceTickets([serviceName]);

        return {
            serviceTicket: serviceTickets[serviceName].ticket,
            userTicket: req.headers['x-ya-user-ticket'],
        };
    } catch (e) {
        throw new Error(`Get service ticket for ${serviceName} error`);
    }
};

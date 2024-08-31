import got from 'got';
import mem from 'mem';
import config from './config';

export const getServiceTickets = mem(
    (dsts: string[], src?: string) => {
        return got(`${config.tvmtool.host}/tvm/tickets?src=${src || config.tvmtool.src}&dsts=${dsts.join(',')}`, {
            headers: { Authorization: config.tvmtool.token },
            json: true,
        }).then((r) => r.body);
    },
    { maxAge: 5 * 60 * 1000 },
);

export const getServiceTicketForBlackbox = () => {
    return getServiceTickets(['blackbox']).then((t) => t.blackbox.ticket);
};

export const getServiceTicketForApi = () => {
    return getServiceTickets([config.tvmtool.src]).then((t) => t[config.tvmtool.src].ticket);
};

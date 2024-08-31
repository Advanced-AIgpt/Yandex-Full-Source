import createGot from '../got';
import config from '../../lib/config';
import { getServiceTickets } from '../tvm';
import merge = require('deepmerge');

const mediaGot = createGot({ sourceName: 'media', baseUrl: config.mediabilling.url });

export const mediaGotWithTvm = async <T = any>(path: string, options: any = {}) => {
    const serviceTicket = await getServiceTickets(['mediabilling']);
    const merged = merge(
        {
            headers: {
                'X-Ya-Service-Ticket': serviceTicket.mediabilling.ticket,
            },
            json: true,
        },
        options,
    );

    const res = await mediaGot(path, merged);
    return (res.body as unknown) as T;
};

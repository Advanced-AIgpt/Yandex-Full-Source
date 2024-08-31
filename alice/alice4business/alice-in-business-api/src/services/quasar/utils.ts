import createGot from '../got';
import { getServiceTickets } from '../tvm';
import config from '../../lib/config';
import merge = require('deepmerge');

const quasarGot = createGot({ sourceName: 'quasar', baseUrl: config.quasar.url });

export const quasarGotWithTvm = async <T = any>(path: string, options?: any) => {
    const serviceTicket = await getServiceTickets(['quasar']);
    const merged = merge(
        {
            headers: {
                'X-Ya-Service-Ticket': serviceTicket.quasar.ticket,
            },
            json: true,
        },
        options,
    );
    return await quasarGot(path, merged).then(({ body }) => (body as unknown) as T);
};

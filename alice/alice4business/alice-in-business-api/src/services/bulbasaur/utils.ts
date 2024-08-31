import createGot from '../got';
import { getServiceTickets } from '../tvm';
import config from '../../lib/config';
import merge = require('deepmerge');

export const bulbasaurGot = createGot({
    sourceName: 'bulbasaur',
    baseUrl: config.bulbasaur.url,
});

export const bulbasaurGotWithTvm = async <T = any>(
    path: string,
    options: object = {},
) => {
    const serviceTicket = await getServiceTickets(['bulbasaur']);
    const mergedOptions = merge(
        {
            headers: {
                'X-Ya-Service-Ticket': serviceTicket.bulbasaur.ticket,
            },
            json: true,
        },
        options,
    );
    return bulbasaurGot(path, mergedOptions).then(({ body }) => (body as unknown) as T);
};

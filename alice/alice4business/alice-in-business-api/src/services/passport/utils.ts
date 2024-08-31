import createGot from '../got';
import { getServiceTickets } from '../tvm';
import config from '../../lib/config';
import merge = require('deepmerge');

export const passportGot = createGot({
    sourceName: 'passport',
    baseUrl: config.passport.host,
    timeout: config.passport.timeout,
});

export const passportGotWithTvm = async <T = any>(path: string, options: object = {}) => {
    const serviceTicket = await getServiceTickets(['passport']);
    const mergedOptions = merge(
        {
            headers: {
                'X-Ya-Service-Ticket': serviceTicket.passport.ticket,
            },
            query: {
                consumer: config.passport.consumer,
            },
            json: true,
        },
        options,
    );
    return passportGot(path, mergedOptions).then(({ body }) => (body as unknown) as T);
};

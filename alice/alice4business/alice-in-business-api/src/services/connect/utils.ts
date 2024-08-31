import createGot from '../got';
import { getServiceTickets } from '../tvm';
import config from '../../lib/config';
import merge = require('deepmerge');

export const connectGot = createGot({
    sourceName: 'connect',
    baseUrl: config.connect.url,
});

export const connectGotWithTvm = async <T = any>(path: string, options: any = {}) => {
    const serviceTicket = await getServiceTickets(['connect'], options.tvmSrc);
    const mergedOptions = merge(
        {
            headers: {
                'X-Ya-Service-Ticket': serviceTicket.connect.ticket,
            },
            json: true,
        },
        options,
    );
    return connectGot(path, mergedOptions).then(({ body }) => (body as unknown) as T);
};

connectGotWithTvm.paginatedResult = async <T = any>(path: string, options: any = {}) => {
    const result = [] as T[];

    do {
        const pageResult = await connectGotWithTvm<{
            links: {
                next?: string;
            };
            result: T[];
        }>(path, options);

        result.push(...pageResult.result);

        path = pageResult.links.next || '';
        delete options.query;
    } while (path);

    return result;
};

export interface User {
    uid: number;
    ip: string;
}

export const _userHeaders = (user: User) => ({
    'X-UID': user.uid,
    'X-User-Ip': user.ip,
});

export const _organizationHeaders = (orgId: number) => ({
    'X-Org-Id': orgId,
});

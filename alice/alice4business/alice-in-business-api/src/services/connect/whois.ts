import { connectGotWithTvm } from './utils';

export const whois = async (
    search: { email?: string; domain?: string } = {},
    options: object = {},
) =>
    connectGotWithTvm<{ org_id: number }[]>(`/v11/who-is/`, {
        ...options,
        method: 'get',
        query: search,
    });

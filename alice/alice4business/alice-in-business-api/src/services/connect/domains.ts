import { _organizationHeaders, _userHeaders, connectGotWithTvm, User } from './utils';

interface Domain {
    name: string;
    master?: boolean;
    owned?: boolean;
    mx?: boolean;
    delegated?: any;
}

export const getDomain = async (
    orgId: number,
    name: string,
    fields: (keyof Domain)[] = ['name'],
    options: object = {},
) =>
    connectGotWithTvm<Domain[]>(`/v11/domains/`, {
        method: 'get',
        query: { fields, name },
        headers: _organizationHeaders(orgId),
        ...options,
    }).then((domains) => domains[0] || undefined);

export const getDomains = async (
    orgId: number,
    fields: (keyof Domain)[] = ['name'],
    options: object = {},
) =>
    connectGotWithTvm<Domain[]>(`/v11/domains/`, {
        method: 'get',
        query: { fields },
        headers: _organizationHeaders(orgId),
        ...options,
    });

export const createDomain = async (
    orgId: number,
    admin: User,
    domain: Partial<Domain>,
    options: object = {},
) =>
    connectGotWithTvm<Domain>('/v11/domains/connect/', {
        ...options,
        method: 'post',
        headers: { ..._organizationHeaders(orgId), ..._userHeaders(admin) },
        body: domain,
    });

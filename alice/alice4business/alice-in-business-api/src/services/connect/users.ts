import { _organizationHeaders, connectGotWithTvm } from './utils';

export interface User {
    id: number;
    email: string;
    nickname: string;
    name: {
        last: string;
        first: string;
        middle?: string;
    };
}

export const getUsers = async (
    orgId: number,
    fields: (keyof User)[] = ['id', 'email', 'name'],
    options: any = {},
) =>
    connectGotWithTvm.paginatedResult<User>('/v11/users/', {
        ...options,
        method: 'get',
        headers: _organizationHeaders(orgId),
        query: { ...(options.query ?? {}), fields },
    });

import { _userHeaders, connectGotWithTvm, User } from './utils';

export interface Organization {
    id: number;
    name: string;
    admin_id: number;
}

export const getOrganizations = async (
    fields: (keyof Organization)[] = ['id', 'name'],
    options: object = {},
) =>
    connectGotWithTvm.paginatedResult<Partial<Organization>>(`/v11/organizations/`, {
        method: 'get',
        query: { fields },
        ...options,
    });

export const getUserOrganizations = async (
    user: User,
    fields: (keyof Organization)[] = ['id', 'name'],
    options: object = {},
) =>
    connectGotWithTvm.paginatedResult<Partial<Organization>>(`/v11/organizations/`, {
        method: 'get',
        query: { fields },
        headers: _userHeaders(user),
        ...options,
    });

export const getOrganization = async (
    orgId: number,
    fields: (keyof Organization)[] = ['id', 'name'],
    options: object = {},
) =>
    connectGotWithTvm<Partial<Organization>>(`/v11/organizations/${orgId}/`, {
        method: 'get',
        query: { fields },
        ...options,
    });

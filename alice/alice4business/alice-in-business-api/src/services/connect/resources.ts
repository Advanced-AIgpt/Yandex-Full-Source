import { _organizationHeaders, _userHeaders, connectGotWithTvm, User } from './utils';

export interface Resource {
    id: string;
    relations?: Relation[];
}

export interface Relation {
    name: string;
    object_type: string;
    object?: {
        id: number;
    };
    object_id?: number;
}

export const getResources = async (
    orgId: number,
    filter: {
        id?: string[];
        relation_name?: string[];
        user_id?: number[];
    } = {},
    options: object = {},
) =>
    connectGotWithTvm.paginatedResult<Resource>(`/v11/resources/`, {
        ...options,
        method: 'get',
        headers: _organizationHeaders(orgId),
        query: filter,
    });

export const getUserResources = async (
    orgId: number,
    user: Partial<User> | Pick<User, 'uid'>,
    filter: {
        id?: string[];
        relation_name?: string[];
    } = {},
    options: object = {},
) => {
    console.log(`/v11/resources/`, {
        ...filter,
        user_id: user.uid,
    });

    return connectGotWithTvm.paginatedResult<Resource>(`/v11/resources/`, {
        ...options,
        method: 'get',
        headers: _organizationHeaders(orgId),
        query: {
            ...filter,
            user_id: user.uid,
        },
    });
};

export const getResource = async (
    orgId: number,
    resourceId: string,
    options: object = {},
) =>
    connectGotWithTvm<Resource>(`/v11/resources/${resourceId}/`, {
        ...options,
        method: 'get',
        headers: _organizationHeaders(orgId),
    });

export const createResource = async (
    orgId: number,
    resource: Resource,
    options: object = {},
) =>
    connectGotWithTvm<Resource>('/v11/resources/', {
        ...options,
        method: 'post',
        headers: _organizationHeaders(orgId),
        body: resource,
    });

export const deleteResource = async (
    orgId: number,
    resourceId: string,
    admin: User,
    options: object = {},
) =>
    connectGotWithTvm<void>(`/v11/resources/${resourceId}/`, {
        ...options,
        method: 'delete',
        headers: {
            ..._organizationHeaders(orgId),
            ..._userHeaders(admin),
        },
    });

export const updateRelations = async (
    orgId: number,
    resourceId: string,
    relations: Relation[],
    user: User,
    options: object = {},
) =>
    connectGotWithTvm<void>(`/v11/resources/${resourceId}/relations/`, {
        ...options,
        method: 'put',
        headers: {
            ..._organizationHeaders(orgId),
            ..._userHeaders(user),
        },
        body: relations,
    });

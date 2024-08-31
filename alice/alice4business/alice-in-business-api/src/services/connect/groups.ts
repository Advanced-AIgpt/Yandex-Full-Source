import { _organizationHeaders, connectGotWithTvm } from './utils';

export interface Group {
    id: number;
    name: string;
    label: string | null;
}

export const getGroups = async (
    orgId: number,
    fields: (keyof Group)[] = ['id', 'name'],
    options: object = {},
) =>
    connectGotWithTvm.paginatedResult<Group>('/v11/groups/', {
        ...options,
        method: 'get',
        headers: _organizationHeaders(orgId),
        query: { fields },
    });

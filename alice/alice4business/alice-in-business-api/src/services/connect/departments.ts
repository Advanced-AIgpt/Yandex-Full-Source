import { _organizationHeaders, connectGotWithTvm } from './utils';

export interface Department {
    id: number;
    name: string;
    label: string | null;
}

export const getDepartments = async (
    orgId: number,
    fields: (keyof Department)[] = ['id', 'name', 'label'],
    options: object = {},
) =>
    connectGotWithTvm.paginatedResult<Department>('/v11/departments/', {
        ...options,
        method: 'get',
        headers: _organizationHeaders(orgId),
        query: { fields },
    });

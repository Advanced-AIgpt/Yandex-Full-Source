import { getAllRoles as getAllRolesService } from '../../services/idm';
import { asyncJsonResponse } from '../utils';

import { RoleError } from './errors';

export const getAllRoles = asyncJsonResponse(async (req: any) => {
    try {
        return { code: 0, users: await getAllRolesService() };
    } catch (e) {
        console.error(e, { message: `Get all roles ${e.errorType}. ${e.message as string}`, scope: 'idm' });
        if (e instanceof RoleError) {
            return { code: e.status, [e.errorType]: e.message };
        }
        throw e;
    }
});

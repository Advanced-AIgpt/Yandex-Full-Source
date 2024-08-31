import { addRole as addRoleService } from '../../services/idm';
import { asyncJsonResponse } from '../utils';

import { RoleError } from './errors';
import { parseRoleParams } from './utils';

export const addRole = asyncJsonResponse(async (req) => {
    try {
        const params = parseRoleParams(req.body);
        await addRoleService(params);
        return { code: 0, data: { 'passport-login': params.login } };
    } catch (e) {
        console.error(e, { message: `Add role ${e.errorType}. ${e.message as string}`, scope: 'idm' });
        if (e instanceof RoleError) {
            return { code: e.status, [e.errorType]: e.message };
        }
        throw e;
    }
});

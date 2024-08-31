import { removeRole as removeRoleService } from '../../services/idm';
import { asyncJsonResponse } from '../utils';

import { RoleError } from './errors';
import { parseRoleParams } from './utils';

export const removeRole = asyncJsonResponse(async req => {
    try {
        const params = parseRoleParams(req.body);
        await removeRoleService(params);
        return { code: 0 };
    } catch (e) {
        if (e.errorType === 'warning') {
            console.warn(e, { message: `Remove role ${e.errorType}. ${e.message as string}`, scope: 'idm' });
        } else {
            console.error(e, { message: `Remove role ${e.errorType}. ${e.message as string}`, scope: 'idm' });
        }
        if (e instanceof RoleError) {
            return { code: e.status, [e.errorType]: e.message };
        }
        throw e;
    }
});

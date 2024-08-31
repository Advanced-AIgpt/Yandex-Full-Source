import { asyncJsonResponse } from '../utils';

import { rolesConfig, fieldsConfig } from './config';

export const getInfo = asyncJsonResponse(async () => {
    return { code: 0, roles: rolesConfig, fields: fieldsConfig };
});

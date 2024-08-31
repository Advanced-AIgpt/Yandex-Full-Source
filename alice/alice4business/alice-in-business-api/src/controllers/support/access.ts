import { asyncJsonResponse } from '../utils';

export const getAccess = asyncJsonResponse(
    async () => {
        return true
    },
    { wrapResult: true },
);

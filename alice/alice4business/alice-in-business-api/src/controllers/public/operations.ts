import RestApiError from '../../lib/errors/restApi';
import { asyncJsonResponse, notFound } from '../utils';
import * as ACL from '../../lib/acl';

export const getOperationInfo = asyncJsonResponse(async (req, res) => {
    const { id } = req.query;
    if (!id || typeof id !== 'string') {
        throw new RestApiError('id parameter is required', 400);
    }
    const op = await ACL.getOperation(req.user, [], { id }).catch(notFound('Operation'));
    return {
        status: 'ok',
        operation: {
            status: op.status,
        },
    };
});

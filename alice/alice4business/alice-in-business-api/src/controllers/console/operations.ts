import { asyncJsonResponse, notFound } from '../utils';
import * as ACL from '../../lib/acl';

export const getOperationInfo = asyncJsonResponse(
    async (req, res) => {
        const { id } = req.params;
        const op = await ACL.getOperation(req.user, [], { id }, { include: ['parent', 'children'] }).catch(
            notFound('Operation'),
        );
        return {
            status: 'ok',
            operation: {
                status: op.status,
                children: op.children!.length > 0 ? op.children?.map(c => ({ id: c.id, status: c.status })) : undefined,
                parent: op.parent ? { id: op.parent!.id, status: op.parent!.status } : undefined
            }
        };
    },
    { wrapResult: true },
);

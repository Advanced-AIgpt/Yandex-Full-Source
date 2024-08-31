import { Operation } from '../../db';
import { asyncJsonResponse, notFound } from '../utils';

export const getOperation = asyncJsonResponse(
    async (req) => {
        const { operationId } = req.params;

        const operation = await Operation.findByPk(operationId, {
            rejectOnEmpty: true,
        }).catch(notFound('Operation'));

        return {
            status: 'ok',
            operation: {
                status: operation.status,
            },
        };
    },
    { wrapResult: true },
);

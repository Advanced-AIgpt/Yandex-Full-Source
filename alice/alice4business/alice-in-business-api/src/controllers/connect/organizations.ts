import { asyncJsonResponse } from '../utils';
import RestApiError from '../../lib/errors/restApi';
import { syncOrganization } from '../../lib/sync/connect';
import log from '../../lib/log';

export const syncOrganizationHandler = asyncJsonResponse(
    async (req, res) => {
        const orgId = parseInt(req.header('x-org-id') ?? '', 10);

        if (isNaN(orgId)) {
            throw new RestApiError('Bad X-Org-Id header value', 400);
        }

        syncOrganization(orgId).catch((error) => {
            log.warn('Connect sync failed', { error });
        });
    },
    {
        putPostStatus: 202,
    },
);

import RestApiError from '../errors/restApi';
import { asyncMiddleware } from '../../controllers/utils';
import log from '../log';
import { checkServiceTicket, checkUserTicket } from '../../services/tvm';
import * as ACL from '../acl';

export const tvmServiceTicketChecker = (permittedTVMAppIds: Set<number> | null) =>
    asyncMiddleware(async (req) => {
        const serviceTicket = req.headers['x-ya-service-ticket'];

        if (typeof serviceTicket !== 'string') {
            log.info(`authentication: x-ya-service-ticket header is missing`);
            throw new RestApiError('Forbidden (no credentials)', 403);
        }

        // - check without authorization -

        const result = await checkServiceTicket(serviceTicket).catch((err) => {
            const message = _extractMessageFromError(err);
            log.warn(`authentication: ${message}`);

            throw new RestApiError(`Forbidden (authentication error)`, 403);
        });

        // - check authorization -
        if (!permittedTVMAppIds) {
            return;
        }

        if (!permittedTVMAppIds.has(result.src)) {
            log.warn(`TVM application is not permitted for access: ${result.src}`);

            throw new RestApiError(`Forbidden (authentication error)`, 403);
        }
    });

export const tvmUserTicketChecker = (throwIfNoUser: boolean) =>
    asyncMiddleware(async (req) => {
        const userTicket = req.headers['x-ya-user-ticket'];

        if (typeof userTicket !== 'string') {
            log.info(`authentication: x-ya-user-ticket header is missing`);
            if (throwIfNoUser) {
                throw new RestApiError('Forbidden (no credentials)', 403);
            }
        } else {
            const result = await checkUserTicket(userTicket).catch((err) => {
                const message = _extractMessageFromError(err);
                log.warn(`authentication: ${message}`);

                throw new RestApiError(`Forbidden (authentication error)`, 403);
            });


            req.user = new ACL.User(result.default_uid, req.ip);
        }
    });

export const accessChecker = () =>
    asyncMiddleware(async (req) => {
        const user = req.user;
        const hasAccess = await ACL.checkSupportCommandAccess(user)

        if (!hasAccess) {
            throw new RestApiError('Forbidden (you have no access rights)', 403);
        }
    });

const _extractMessageFromError = (err: any) => {
    return err.response?.body.error ?? err.response?.body ?? err.message;
};

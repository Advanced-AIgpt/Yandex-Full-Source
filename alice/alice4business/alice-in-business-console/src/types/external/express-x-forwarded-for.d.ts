declare module '@yandex-int/express-x-forwarded-for-fix' {
    import { RequestHandler } from 'express';

    export interface ExpressXForwardedForFixOptions {
        /** Default IP if others has been filtered (useful for debug) */
        defaultIP?: string;

        /**
         * Filter reserved IPv4s
         *
         * See:
         * https://github.yandex-team.ru/search-interfaces/frontend/blob/master/packages/express-x-forwarded-for-fix/index.js
         */
        filterReserved?: boolean;
    }

    /**
     * Filters inccorect IPs from x-forwarded-for header
     *
     * See:
     * https://github.yandex-team.ru/search-interfaces/frontend/tree/master/packages/express-x-forwarded-for-fix
     */
    function expressXForwardedForFix(options?: ExpressXForwardedForFixOptions): RequestHandler;

    export default expressXForwardedForFix;
}

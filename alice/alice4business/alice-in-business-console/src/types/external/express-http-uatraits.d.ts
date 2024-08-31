declare namespace Express {
    interface Request {
        /**
         * Parsed user agent header by express-http-uatraits middleware
         *
         * See:
         * https://github.yandex-team.ru/toolbox/express-http-uatraits
         */
        uatraits: Record<string, any>;
    }
}

declare module '@yandex-int/express-http-uatraits' {
    import { GotOptions } from 'got';
    import { RequestHandler } from 'express';

    namespace e {
        export interface ExpressHttpUatraitsOptions {
            clientOptions?: GotOptions<string | null>;

            /** API endpoint (http://uatraits.qloud.yandex.ru by default) */
            server?: string;
        }
    }

    /**
     * Parses user agent header into req.uatraits
     *
     * See:
     * https://github.yandex-team.ru/toolbox/express-http-uatraits
     */
    function e(options?: e.ExpressHttpUatraitsOptions): RequestHandler;

    export = e;
}

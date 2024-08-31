declare namespace Express {
    export interface Request {
        /**
         * Generated secretkey
         *
         * See:
         * https://github.yandex-team.ru/toolbox/express-secretkey
         */
        secretkey?: string;
    }
}

declare module '@yandex-int/express-secretkey' {
    import { RequestHandler } from 'express';

    namespace e {
        export interface ExpressSecretkeyOptions {
            /**
             * HTTP methods which will be ignored when checking secret token
             *
             * Default: ['GET', 'HEAD', 'OPTIONS']
             */
            ignoreMethods?: string[];

            /**
             * Function to get secret token from request object
             *
             * Default:
             * ```
             * req => (req.body && req.body.sk) ||
             *     (req.query && req.query.sk) ||
             *     req.headers['csrf-token'] ||
             *     req.headers['xsrf-token'] ||
             *     req.headers['x-csrf-token'] ||
             *     req.headers['x-xsrf-token'];
             * ```
             */
            value?: (req: Request) => string;
        }
    }

    interface SecretkeyMiddleware extends RequestHandler {
        validate: RequestHandler;
    }

    /**
     * Generate secret token into req.secretkey and validate it from headers
     *
     * See:
     * https://github.yandex-team.ru/toolbox/express-secretkey
     */
    function e(options?: e.ExpressSecretkeyOptions): SecretkeyMiddleware;

    export = e;
}

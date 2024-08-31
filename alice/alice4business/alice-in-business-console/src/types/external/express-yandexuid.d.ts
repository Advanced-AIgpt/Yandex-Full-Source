declare module '@yandex-int/express-yandexuid' {
    import { RequestHandler } from 'express';

    namespace e {
        export interface ExpressYandexuidOptions {
            /** Cookie expires directive (10 years by default) */
            expires?: number;

            /** Cookie domain directive (.yandex.{tld} by default) */
            yaDomain?: string;
        }
    }

    /**
     * Sets yandexuid cookie
     *
     * See:
     * https://github.yandex-team.ru/toolbox/express-yandexuid
     */
    function e(options?: e.ExpressYandexuidOptions): RequestHandler;

    export = e;
}

declare module '@yandex-int/express-domain-access' {
    import express from 'express';

    function e(domainsList: string | string[]): express.RequestHandler;

    export = e;
}

declare namespace Express {
    interface Request {
        nonce: string;
    }
}

declare module '@yandex-int/express-yandex-csp';

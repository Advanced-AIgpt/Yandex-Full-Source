declare namespace xiva {
    export interface Options {
        api?: string;
        urlParams?: object;
    }

    export interface EventDetail {
        operation: string;
        message: string;
    }

    export interface Instance extends EventTarget {
        reconnect(urlParams: object, fullRewrite?: boolean): void;
        destruct(): void;
        send(message: string): void;
    }
}

interface Window {
    Ya: {
        XIVA: new (params?: xiva.Options) => xiva.Instance;
    };
}

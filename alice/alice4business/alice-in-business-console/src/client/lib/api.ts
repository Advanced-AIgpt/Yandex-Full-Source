import { rtrim } from './utils';

export interface ApiParams {
    rootUrl: string;
    secretkey: string;

    pollingInterval: number;
}

type Method = 'DELETE' | 'POST' | 'GET' | 'PATCH' | 'HEAD';

interface JsonDict {
    [key: string]: any;
}

export interface ApiErrorType {
    name: string;
    message: string;
    code: number;
    fields?: any;
    payload?: any;
}

interface ApiErrorParams {
    code: number;
    payload?: ApiErrorType['payload'];
    fields?: ApiErrorType['fields'];
}

export class ApiError extends Error implements ApiErrorType {
    public code: number;
    public fields: ApiErrorType['fields'];
    public payload: ApiErrorType['payload'];

    constructor(public message: string, params: ApiErrorParams) {
        super(message);
        this.code = params.code;
        this.fields = params.fields;
        this.payload = params.payload;
        Object.setPrototypeOf(this, ApiError.prototype);
    }
}

export default class API {
    constructor({ rootUrl, secretkey, pollingInterval }: ApiParams) {
        this.rootUrl = rtrim('/', rootUrl);
        this.secretkey = secretkey;

        this.pollingInterval = pollingInterval;
    }

    protected rootUrl: string;
    private secretkey: string;

    public readonly pollingInterval: number;

    protected call = <T = void>(method: Method, url: string, params?: FormData | JsonDict): Promise<T> => {
        const headers = new Headers({
            'x-csrf-token': this.secretkey,
            ...(!(params instanceof FormData) && {
                Accept: 'application/json',
                'Content-Type': 'application/json',
            }),
        });
        // const body = !params || params instanceof FormData ? params : JSON.stringify(params);

        const body =
            method === 'GET' ? undefined : !params || params instanceof FormData ? params : JSON.stringify(params);

        return fetch(url, {
            body,
            credentials: 'include',
            headers,
            method,
        })
            .then((res) => {
                if (res.status === 401) {
                    throw new ApiError('Unauthorized', { code: 401 });
                }
                if (res.status === 403) {
                    throw new ApiError('Forbidden', { code: 403 });
                }
                return res.json();
            })
            .then((res) => {
                if (res.status === 'error' || res.error) {
                    const { message, code, fields } = res.error;
                    const { payload } = res;
                    throw new ApiError(message, { code, fields, payload });
                }
                return res.result as T;
            });
    };

    public static isError = (res: any): res is ApiError => {
        return res instanceof ApiError;
    };
    public static is5xxError = (error: ApiError) => error.code >= 500;
    public static is4xxError = (error: ApiError) => error.code >= 400 && error.code < 500;
    public static isUniqueConstrainError = (error: any) => {
        return (
            Boolean(error) &&
            error.message.toLowerCase().indexOf('unique') > -1 &&
            error.message.toLowerCase().indexOf('constrai') > -1
        );
    };
}

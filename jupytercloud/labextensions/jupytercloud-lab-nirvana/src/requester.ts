import { PartialJSONObject } from '@lumino/coreutils';
import { URLExt } from '@jupyterlab/coreutils';
import { IJupyterCloud } from 'jupytercloud-lab-lib';

export type TRequestErrorCallback = (error: IResponseParseResult) => void;

export interface IHubRequester {
    jupyterCloud: IJupyterCloud;
    errorCallback: TRequestErrorCallback;
}

export interface IHubRequesterRequest {
    path: string;
    method: string;
    data?: any;
    query?: PartialJSONObject;
}

export interface IResponseParseResult {
    json: any;
    text: string;
    status: number;
    statusText: string;
}

export class HubRequester {
    constructor(private props: IHubRequester) {}

    protected getHubUrl = (path: string) => {
        return this.props.jupyterCloud.getHubUrl(path);
    };

    protected parseResponse = async (
        response: Response
    ): Promise<IResponseParseResult> => {
        const text = await response.text();
        let json;
        try {
            json = JSON.parse(text);
        } catch (e) {
            console.warn('error parsing response JSON', e, text);
        }

        return {
            json,
            text,
            status: response.status,
            statusText: response.statusText
        };
    };

    public request = ({ path, method, data, query }: IHubRequesterRequest) => {
        const url =
            this.getHubUrl(path) +
            (query ? URLExt.objectToQueryString(query) : '');
        const body = data ? JSON.stringify(data) : null;
        const request = new Request(url, { method, body });
        return fetch(request)
            .then(this.parseResponse)
            .then(result => {
                // TODO: use normal request library
                const statusType = Math.floor(result.status / 100);
                if ((statusType !== 2 && statusType !== 3) || !result.json) {
                    return Promise.reject(result);
                }
                return Promise.resolve(result.json);
            })
            .catch(error => {
                this.props.errorCallback(error);
                return Promise.reject('error requesting jupyterhub');
            });
    };
}

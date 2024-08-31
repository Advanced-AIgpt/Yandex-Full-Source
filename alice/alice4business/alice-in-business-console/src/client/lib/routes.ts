import { rtrim, serializeQueryParams } from './utils';

export interface IRoutes {
    app: {
        root(): string;
        customer(path?: string): string;
        organization(organizationId: string): string;
        deviceList(organizationId: string): string;
        roomList(organizationId: string): string;
        history(organizationId: string): string;
        settings(organizationId: string): string;
        deviceHistory(organizationId: string, devicePk: string): string;
    };
    assets(asset: string): string;
    api(route: string, query?: string | Record<string, string | undefined>): string;

    connect: {
        portal(connectOrgId: number): string;
        settings(connectOrgId: number): string;
    };

    legal: {
        userAgreement: string;
        privacyPolicy: string;
        licenseAgreement: string;
    };
}

export interface RoutesParams {
    appUrl: string;
    assetsUrl: string;
    apiUrl: string;
    connectUrl: string;
    secretkey: string;
}

export default class Routes implements IRoutes {
    constructor({ appUrl, assetsUrl, apiUrl, connectUrl, secretkey }: RoutesParams) {
        this._appUrl = rtrim('/', appUrl);
        this._assetsUrl = rtrim('/', assetsUrl);
        this._apiUrl = rtrim('/', apiUrl);
        this._connectUrl = rtrim('/', connectUrl);
        this._secretkey = secretkey;
    }

    private readonly _appUrl: string;
    private readonly _assetsUrl: string;
    private readonly _apiUrl: string;
    private readonly _connectUrl: string;
    private readonly _secretkey: string;

    public app = {
        root: () => this._appUrl,
        customer: (path?: string) => `${this._appUrl}/customer${path ? `/${path}` : ''}`,
        organization: (organizationId: string) => `${this._appUrl}/devices/${organizationId}`,
        deviceList: (organizationId: string) => `${this._appUrl}/devices/${organizationId}/list`,
        roomList: (organizationId: string) => `${this._appUrl}/devices/${organizationId}/rooms`,
        history: (organizationId: string) => `${this._appUrl}/devices/${organizationId}/history`,
        settings: (organizationId: string) => `${this._appUrl}/devices/${organizationId}/settings`,
        deviceHistory: (organizationId: string, devicePk: string) =>
            `${this._appUrl}/devices/${organizationId}/history/${devicePk}`,
    };

    public assets = (asset: string) => `${this._assetsUrl}/assets/${asset}`;

    public api = (route: string, query?: string | Record<string, string | undefined>) =>
        `${this._apiUrl}/${route}${
            query
                ? '?' +
                  (typeof query === 'string' ? query.replace('?', '') : serializeQueryParams(query))
                : ''
        }`;

    private connectSwitchOrg = (connectOrgId: number, retPath: string) =>
        `${this._connectUrl}/portal/context` +
        `?org_id=${encodeURIComponent(connectOrgId.toString(10))}` +
        `&sk=${encodeURIComponent(this._secretkey)}` +
        (retPath ? `&retpath=${encodeURIComponent(retPath)}` : '');

    public connect = {
        portal: (connectOrgId: number) =>
            this.connectSwitchOrg(connectOrgId, `${this._connectUrl}/portal/home`),
        settings: (connectOrgId: number) =>
            this.connectSwitchOrg(connectOrgId, `${this._connectUrl}/portal/services/alice_b2b`),
    };

    public legal = {
        userAgreement: 'https://yandex.ru/legal/rules/',
        privacyPolicy: 'https://yandex.ru/legal/confidential/',
        licenseAgreement: 'https://yandex.ru/legal/search_mobile_agreement/',
    };
}

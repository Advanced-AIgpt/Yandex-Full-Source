import { ParamsType } from '../pages/Support/SupportPage';

import API from './api';

type GetResponseType = Array<Record<'value' | 'name', string>>;
interface PostResponseType {
    status: 'ok' | 'not ok',
    message?: string
}

export interface ISupportApi extends API {
    getAccess(): Promise<boolean>;

    getAllDevices(): Promise<GetResponseType>;
    getOrganizationDevices(id: string): Promise<GetResponseType>;
    getDeviceDefaults(id: string): Promise<ParamsType>;
    changeDevice(params: ParamsType): Promise<PostResponseType>;
    addPuid(params: ParamsType): Promise<PostResponseType>;
    createDevice(params: ParamsType): Promise<PostResponseType>;

    getAllOrganizations(): Promise<GetResponseType>;
    getOrganizationDefaults(id: string): Promise<ParamsType>;
    createOrganization(params: ParamsType): Promise<PostResponseType>;
    changeOrganization(params: ParamsType): Promise<PostResponseType>;
    getConnectOrganizations(): Promise<GetResponseType>;
    setOrganizationMaxVolume(params: ParamsType): Promise<PostResponseType>;

    addPromocode(params: ParamsType): Promise<PostResponseType>;
    addPromocodeToOrganization(params: ParamsType): Promise<PostResponseType>;

    createRoom(params: ParamsType): Promise<PostResponseType>;

    createUser(params: ParamsType): Promise<PostResponseType>;
    bindUsers(params: ParamsType): Promise<PostResponseType>;
    getAllUsers(): Promise<GetResponseType>;

}

export default class SupportAPI extends API implements ISupportApi {
    public getAccess = () => this.call<boolean>('GET', `${this.rootUrl}/access`);

    public getAllDevices = () => this.call<GetResponseType>('GET', `${this.rootUrl}/devices/all`);
    public getDeviceDefaults = (id: string) => this.call<ParamsType>('GET', `${this.rootUrl}/devices/${id}/defaults`);
    public changeDevice = (params: ParamsType) => this.call<PostResponseType>('POST', `${this.rootUrl}/devices/change`, {params});
    public addPuid = (params: ParamsType) => this.call<PostResponseType>('POST', `${this.rootUrl}/devices/addPuid`, {params});
    public createDevice = (params: ParamsType) =>
        this.call<PostResponseType>('POST', `${this.rootUrl}/devices/create`, { params });

    public getAllOrganizations = () => this.call<GetResponseType>('GET', `${this.rootUrl}/organizations/all`)
    public getOrganizationDevices = (id: string) => this.call<GetResponseType>('GET', `${this.rootUrl}/organizations/${id}/devices`);
    public getOrganizationDefaults = (id: string) => this.call<ParamsType>('GET', `${this.rootUrl}/organizations/${id}/defaults`)
    public createOrganization = (params: ParamsType) => this.call<PostResponseType>('POST', `${this.rootUrl}/organizations/create`, {params})
    public changeOrganization = (params: ParamsType) => this.call<PostResponseType>('POST', `${this.rootUrl}/organizations/change`, {params})
    public getConnectOrganizations = () => this.call<GetResponseType>('GET', `${this.rootUrl}/organizations/getConnect`)
    public setOrganizationMaxVolume = (params: ParamsType) => this.call<PostResponseType>('POST', `${this.rootUrl}/organizations/setMaxVolume`, {params})

    public addPromocode = (params: ParamsType) => this.call<PostResponseType>('POST', `${this.rootUrl}/promocodes/add`, {params});
    public addPromocodeToOrganization = (params: ParamsType) => this.call<PostResponseType>('POST', `${this.rootUrl}/promocodes/addToOrganization`, {params});

    public createRoom = (params: ParamsType) => this.call<PostResponseType>('POST', `${this.rootUrl}/rooms/create`, {params});

    public createUser = (params: ParamsType) => this.call<PostResponseType>('POST', `${this.rootUrl}/users/create`, {params});
    public bindUsers = (params: ParamsType) => this.call<PostResponseType>('POST', `${this.rootUrl}/users/bind`, {params});
    public getAllUsers = () => this.call<GetResponseType>('GET', `${this.rootUrl}/users/all`);
}

import { IConnectOrganization } from './connectOrganization';

export interface IOrganization {
    name: string;
    id: string;
    promoCount: number;
    connectOrganization?: IConnectOrganization;
    usesRooms: boolean;
}

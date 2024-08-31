import { IColorSet } from '../../style/colorSet';

export enum EnumTravelTypes {
    car,
    publicTransport,
    onFoot,
    undefined
}

export interface IRoute {
    value: string;
    comment?: string;
    type: EnumTravelTypes;
    map: string;
}

export interface IRouteResponseData {
    from: string;
    to: string;
    routes: IRoute[];
    colorSet: IColorSet;
}

import { EnumTravelTypes, IRoute, IRouteResponseData } from './types';
import getColorSet from '../../style/colorSet';
import { NAlice } from '../../../../protos';
import { compact } from 'lodash';
import MapsStaticAPI from '../../../../common/helpers/mapsStaticAPI/mapsStaticAPI';
import { IRequestState } from '../../../../common/types/common';
import { getEnvs } from '../../../../env';
import { logger } from '../../../../common/logger';
import { AdapterMinimalDataError } from '../../helpers/AdapterMinimalDataError';

type ITRouteLocation = NAlice.NData.ITRouteLocation;
type EType = NAlice.NData.TRoute.EType;

type ITRoute = NAlice.NData.ITRoute;
type ITShowRouteData = NAlice.NData.ITShowRouteData;

function getRouteType(routeType: EType): EnumTravelTypes {
    switch (routeType) {
        case NAlice.NData.TRoute.EType.CAR:
            return EnumTravelTypes.car;
        case NAlice.NData.TRoute.EType.PEDESTRIAN:
            return EnumTravelTypes.onFoot;
        case NAlice.NData.TRoute.EType.PUBLIC_TRANSPORT:
            return EnumTravelTypes.publicTransport;
        case NAlice.NData.TRoute.EType.UNDEFINED:
            return EnumTravelTypes.undefined;
    }
}

function getComment(type: EnumTravelTypes, lengthText: string): string {
    switch (type) {
        case EnumTravelTypes.onFoot:
            return `Пройти ${lengthText} пешком`;
        case EnumTravelTypes.car:
            return `На машине ${lengthText}`;
        case EnumTravelTypes.publicTransport:
            return `На транспорте ${lengthText}`;
        case EnumTravelTypes.undefined:
            return lengthText;
    }
}

function routeDataAdapter(data: ITRoute, requestState: IRequestState): IRoute | null {
    if (!data.Type) {
        logger.error(new AdapterMinimalDataError(['Routes[].Type'], 'RouteResponse'));
        return null;
    }

    let map: string;
    if (data.ImageUri) {
        const tmpMap = new URL(data.ImageUri);
        map = (new MapsStaticAPI(tmpMap, getEnvs().SECRET_MAPS_API_KEY, getEnvs().SECRET_MAPS_SIGNING_SECRET))
            .setSize(requestState.sizes.width/2, requestState.sizes.height)
            .setTheme('dark')
            .toString();
    } else {
        logger.error(new AdapterMinimalDataError(['Routes[].Image'], 'RouteResponse'));
        map = ' ';
    }

    const type = getRouteType(data.Type);

    return {
        map,
        value: data.Time?.Text || ' ',
        type,
        comment: getComment(type, data.Length?.Text || ''),
    };
}

function getAddress(data: ITRouteLocation | null | undefined): string {
    const house = data?.ResolvedLocation?.Geo?.House;
    const street = data?.ResolvedLocation?.Geo?.Street;
    const city = data?.ResolvedLocation?.Geo?.City;
    const name = data?.ResolvedLocation?.Name || data?.ResolvedLocation?.CompanyName;
    if (data?.NamedLocation === 'home') {
        return 'Дом';
    }
    if (data?.NamedLocation === 'work') {
        return 'Работа';
    }
    if (name) {
        return name;
    }
    if (house || street) {
        return compact([street, house]).join(', ');
    }
    if (city) {
        return city;
    }
    return data?.ResolvedLocation?.Geo?.AddressLine || ' ';
}

export function dataAdapter(data: ITShowRouteData, requestState: IRequestState): IRouteResponseData {
    return {
        from: getAddress(data.From),
        to: getAddress(data.To),
        colorSet: getColorSet(),
        routes: compact(data.Routes?.map(el => routeDataAdapter(el, requestState))) || [],
    };
}

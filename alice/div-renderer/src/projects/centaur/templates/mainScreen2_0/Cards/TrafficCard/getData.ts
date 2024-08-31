import { compact } from 'lodash';
import { ITrafficCardProps, ITrafficForecast } from './types';
import { NAlice } from '../../../../../../protos';
import { sortTimeOfDay } from './common';
import { ICardDataAdapter } from '../types';

function forecastDataAdapter(forecastItem: NAlice.NData.ITTrafficForecastData | null | undefined): ITrafficForecast | null {
    if (!forecastItem || forecastItem.Hour === null || typeof forecastItem.Hour === 'undefined') {
        return null;
    }

    return {
        time: forecastItem.Hour,
        trafficValue: forecastItem.Score,
    };
}

export const getTrafficCardData: ICardDataAdapter<ITrafficCardProps> =
    function getTrafficCardData(card, requestState) {
        if (typeof card.TrafficCardData !== 'undefined' && card.TrafficCardData !== null) {
            const forecasts = compact(card.TrafficCardData.Forecast?.map(forecastDataAdapter));
            const sortedForecasts = sortTimeOfDay(forecasts).slice(0, 4);

            return {
                type: 'traffic',
                text: card.TrafficCardData.Message,
                trafficValue: card.TrafficCardData.Score,
                city: card.TrafficCardData.City,
                requestState,
                forecasts: sortedForecasts,
            };
        }
        return null;
    };

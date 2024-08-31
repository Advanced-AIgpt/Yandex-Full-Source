import { compact } from 'lodash';
import { IWeatherCardProps } from './types';
import { calcCondition, calcDaypart, getWeatherImage } from '../../../weather/common';
import { EnumLayer } from '../../../../actions/client';
import { ICardDataAdapter } from '../types';

export const getWeatherCardData: ICardDataAdapter<IWeatherCardProps> = function getWeatherCardData(card, requestState) {
    if (typeof card.WeatherCardData !== 'undefined' && card.WeatherCardData !== null) {
        const data = card.WeatherCardData;

        return {
            type: 'weather',
            temperature: data.Temperature,
            image: data.Image,
            comment: data.Comment,
            city: data.City,
            bgImage: data.Sunrise &&
                data.Sunrise &&
                data.UserTime ?
                getWeatherImage(
                    calcDaypart(data.Sunrise, data.Sunrise, data.UserTime),
                    calcCondition(data.Condition?.Cloudness ?? 0, data.Condition?.PrecStrength ?? 0),
                ) : undefined,
            layer: EnumLayer.dialog,
            requestState,
            actions: compact([
                card.Action && {
                    log_id: 'weather_action',
                    url: card.Action,
                },
            ]),
        };
    }
    return null;
};

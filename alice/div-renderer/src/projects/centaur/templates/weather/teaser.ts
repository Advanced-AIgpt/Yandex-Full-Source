import { TemplateCard, Templates } from 'divcard2';
import { NAlice } from '../../../../protos';
import { calcCondition, calcDaypart, container, hoursGallery, nowBlock, weatherTemplates } from './common';
import { MMRequest } from '../../../../common/helpers/MMRequest';
import { ExpFlags, hasExperiment } from '../../expFlags';
import { WeatherTeaser } from '../teasers/weather/WeatherTeaser';
import { IRequestState } from '../../../../common/types/common';

export const weatherRenderTeaser = (data: NAlice.NData.ITWeatherTeaserData, mmRequest: MMRequest, requestState: IRequestState) => {
    if (hasExperiment(mmRequest, ExpFlags.teasersDesignWithDoubleScreenWeather)) {
        return WeatherTeaser(data, mmRequest, requestState);
    }

    const {
        Sunset,
        Sunrise,
        UserTime,
        Condition,
        GeoLocation,
        Temperature,
        IconType,
        HourItems,
    } = data;
    const daypart = calcDaypart(Sunrise, Sunset, UserTime);
    const condition = calcCondition(Condition?.Cloudness ?? 0, Condition?.PrecStrength ?? 0);
    return new TemplateCard(new Templates(weatherTemplates), {
        log_id: 'weather.teasers',
        states: [
            {
                state_id: 0,
                div: container({
                    daypart,
                    condition,
                    items: [
                        nowBlock({
                            cityFormed: GeoLocation?.CityPrepcase ?? '',
                            feelsLike: Condition?.FeelsLike ?? 0,
                            temp: Temperature ?? 0,
                            iconType: IconType ?? '',
                            title: Condition?.Title ?? '',
                            when: 'Сейчас',
                        }),
                        hoursGallery({
                            hours: HourItems ?? [],
                        }),
                    ],
                }),
            },
        ],
    });
};

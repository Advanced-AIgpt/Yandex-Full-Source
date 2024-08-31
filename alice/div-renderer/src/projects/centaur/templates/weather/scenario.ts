import {
    ContainerBlock,
    Div,
    FixedSize,
    ImageBlock,
    MatchParentSize,
    TemplateCard,
    Templates,
    TextBlock,
    WrapContentSize,
} from 'divcard2';
import { NAlice } from '../../../../protos';
import { formatDate } from '../../helpers/helpers';
import { SuggestsBlock } from '../suggests';
import {
    calcCondition,
    calcDaypart,
    container,
    dateRange,
    dayParts,
    dayPartsForms,
    daysGallery,
    formatTemp,
    hoursGallery,
    nowBlock,
    formWeatherIconUrl,
    weatherTemplates,
    calcWhen,
    isWeekend,
    weekendCast,
} from './common';
import { text40m, title240m, title48r } from '../../style/Text/Text';
import { colorWhiteOpacity50 } from '../../style/constants';
import { MMRequest } from '../../../../common/helpers/MMRequest';
import { CloseButtonWrapper } from '../../components/CloseButtonWrapper/CloseButtonWrapper';

const wrapSuggests = (suggests: Div) => new ContainerBlock({
    width: new MatchParentSize(),
    height: new MatchParentSize(),
    items: [suggests],
    alignment_vertical: 'bottom',
    content_alignment_vertical: 'bottom',
    paddings: {
        left: 48,
        right: 48,
        bottom: 48,
    },
});

export const weatherRenderDayHours = ({
    Condition,
    UserTime,
    UserDate,
    Date: date,
    Sunrise,
    Temperature,
    Sunset,
    HourItems,
    IconType,
    GeoLocation,
    DayPartType,
}: NAlice.NData.ITWeatherDayHoursData, mmRequest: MMRequest) => {
    const daypart = calcDaypart(Sunrise, Sunset, UserTime);
    const condition = calcCondition(Condition?.Cloudness ?? 0, Condition?.PrecStrength ?? 0);
    const when = calcWhen({
        userDate: new Date(UserDate ?? 0),
        date: new Date(date ?? 0),
        daypart: DayPartType ?? undefined,
    });
    return new TemplateCard(new Templates(weatherTemplates), {
        log_id: 'weather_card',
        states: [
            {
                state_id: 0,
                div: CloseButtonWrapper({
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
                                when,
                            }),
                            hoursGallery({
                                hours: HourItems ?? [],
                            }),
                            wrapSuggests(SuggestsBlock(mmRequest.ScenarioResponseBody)),
                        ],
                    }),
                }),
            },
        ],
    });
};

export const weatherRenderDay = ({
    DayPartItems,
    Sunrise,
    Date: DateStr,
    Sunset,
    UserTime,
    Condition,
    GeoLocation,
    Temperature,
    IconType,
}: NAlice.NData.ITWeatherDayData, mmRequest: MMRequest) => {
    const daypart = calcDaypart(Sunrise, Sunset, UserTime);
    const condition = calcCondition(Condition?.Cloudness ?? 0, Condition?.PrecStrength ?? 0);
    return new TemplateCard(new Templates(weatherTemplates), {
        log_id: 'weather-day',
        states: [
            {
                state_id: 0,
                div: CloseButtonWrapper({
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
                                when: formatDate(DateStr ?? 0, 'd MMMM '),
                            }),
                            dayParts(DayPartItems ?? []),
                            wrapSuggests(SuggestsBlock(mmRequest.ScenarioResponseBody)),
                        ],
                    }),
                }),
            },
        ],
    });
};

export const weatherRenderPartDay = ({
    Condition,
    GeoLocation,
    Temperature,
    IconType,
    DayPartType,
    Date: DateStr,
}: NAlice.NData.ITWeatherDayPartData, mmRequest: MMRequest) => {
    const daypart = DayPartType ?? 'morning';
    const condition = calcCondition(Condition?.Cloudness ?? 0, Condition?.PrecStrength ?? 0);
    return new TemplateCard(new Templates(weatherTemplates), {
        log_id: 'weather-day-part',
        states: [
            {
                state_id: 0,
                div: CloseButtonWrapper({
                    div: container({
                        daypart,
                        condition,
                        items: [
                            new ContainerBlock({
                                width: new WrapContentSize(),
                                height: new MatchParentSize(),
                                alignment_horizontal: 'center',
                                content_alignment_vertical: 'center',
                                items: [
                                    new TextBlock({
                                        ...title48r,
                                        text:
                                            formatDate(DateStr ?? 0, 'd MMMM ') +
                                            ' ' +
                                            dayPartsForms[daypart] +
                                            ' ' +
                                            GeoLocation?.CityPrepcase,
                                        height: new WrapContentSize(),
                                        width: new WrapContentSize(),
                                    }),
                                    new ContainerBlock({
                                        height: new WrapContentSize(),
                                        width: new WrapContentSize(),
                                        orientation: 'horizontal',
                                        content_alignment_vertical: 'center',
                                        items: [
                                            new TextBlock({
                                                ...title240m,
                                                text: formatTemp(Temperature ?? 0),
                                                height: new WrapContentSize(),
                                                width: new WrapContentSize(),
                                            }),
                                            new ImageBlock({
                                                image_url: formWeatherIconUrl(IconType ?? '', 220),
                                                width: new FixedSize({ value: 220 }),
                                                height: new FixedSize({ value: 220 }),
                                            }),
                                        ],
                                    }),
                                    new TextBlock({
                                        ...title48r,
                                        text_color: colorWhiteOpacity50,
                                        text: `${Condition?.Title}, ощущается как ${formatTemp(Condition?.FeelsLike ?? 0)}`,
                                        height: new WrapContentSize(),
                                        width: new WrapContentSize(),
                                    }),
                                ],
                            }),
                            wrapSuggests(SuggestsBlock(mmRequest.ScenarioResponseBody)),
                        ],
                    }),
                }),
            },
        ],
    });
};

export const weatherRenderDaysRange = ({
    DayItems,
    UserTime,
    TodayDaylight,
    GeoLocation,
}: NAlice.NData.ITWeatherDaysRangeData, mmRequest: MMRequest) => {
    const condition = 'clear';
    const daypart = calcDaypart(TodayDaylight?.Sunrise, TodayDaylight?.Sunset, UserTime);

    return new TemplateCard(new Templates(weatherTemplates), {
        log_id: 'weather-days-range',
        states: [
            {
                state_id: 0,
                div: CloseButtonWrapper({
                    div: container({
                        condition,
                        daypart,
                        items: [
                            new TextBlock({
                                ...text40m,
                                text: `Погода ${ GeoLocation?.CityPrepcase} ${ dateRange(DayItems ?? [])}`,
                                margins: {
                                    top: 48,
                                    left: 48,
                                },
                                width: new WrapContentSize(),
                            }),
                            isWeekend(DayItems) ? weekendCast(DayItems) : daysGallery(DayItems ?? []),
                            wrapSuggests(SuggestsBlock(mmRequest.ScenarioResponseBody)),
                        ],
                    }),
                }),
            },
        ],
    });
};

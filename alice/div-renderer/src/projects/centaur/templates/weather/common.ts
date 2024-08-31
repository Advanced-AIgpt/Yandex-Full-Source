import {
    ContainerBlock,
    Div, FixedSize,
    GalleryBlock, ImageBackground,
    ImageBlock, MatchParentSize, SolidBackground,
    Template,
    templateHelper,
    TextBlock,
    WrapContentSize,
} from 'divcard2';
import { capitalize } from 'lodash';
import { differenceInCalendarDays } from 'date-fns';
import { NAlice } from '../../../../protos';
import { Void } from '../../../../common/types/util';
import { formatDate, pluralize } from '../../helpers/helpers';
import { text40m, title146m, title48m, title56m, title64m, title90m } from '../../style/Text/Text';
import { colorWhiteOpacity40, colorWhiteOpacity50, colorWhiteOpacity53 } from '../../style/constants';
import { getS3Asset } from '../../helpers/assets';

const s3Url = (part: string) => getS3Asset(`weather/${part}`);
export const getWeatherImage = (daypart: string, condition: string) => s3Url('fact_bg_' + daypart + '_' + condition) + '.png';

const cloudiness = (cloudiness: number, precStrength: number) => {
    if (cloudiness < 0.01 && precStrength < 0.01) {
        return 'clear';
    }

    if (precStrength < 0.01) {
        if (cloudiness > 0.75) {
            return 'overcast_light_prec';
        }
        return 'cloudy';
    }

    if (precStrength < 0.5) {
        return 'overcast_light_prec';
    }

    return 'overcast_prec';
};

export const dayPartsForms: Record<string, string> = {
    morning: 'утром',
    day: 'днем',
    evening: 'вечером',
    night: 'ночью',
};
export const formatTemp = (temp: number) => `${temp > 0 ? '+' : ''}${temp}°`;

export const formatHour = (hour: number) => `${hour < 10 ? '0' : ''}${hour}:00`;

export const calcDaypart = (sunrise: Void<string>, sunset: Void<string>, userTime: Void<string>) =>
    sunrise && sunset && userTime && userTime > sunrise && userTime < sunset ? 'day' : 'night';

export const calcCondition = (cloudness: number, precStrength: number) => cloudiness(cloudness, precStrength);

export const formWeatherIconUrl = (iconType: string, size: number) => s3Url(`${iconType}_${size}.png`);

type CalcWhenProps = {
    userDate: Date;
    date: Date;
    daypart: string | undefined;
};
export const calcWhen = ({ userDate, date, daypart }: CalcWhenProps) => {
    const result: string[] = [];
    result.push(['Сейчас', 'Завтра', 'Послезавтра'][differenceInCalendarDays(userDate, date)]);
    if (daypart) {
        result.push(dayPartsForms[daypart]);
    }
    return result.join(' ');
};

export const weatherTemplates = {
    weatherCard: new ContainerBlock({
        width: new FixedSize({ value: 150 }),
        height: new FixedSize({ value: 300 }),
        orientation: 'vertical',
        content_alignment_horizontal: 'center',
        items: [
            new TextBlock({
                ...text40m,
                width: new WrapContentSize(),
                margins: {
                    top: 24,
                },
                text: new Template('time'),
            }),
            new ImageBlock({
                width: new FixedSize({ value: 100 }),
                height: new FixedSize({ value: 100 }),
                margins: {
                    top: 20,
                },
                image_url: new Template('icon'),
            }),
            new TextBlock({
                ...title48m,
                width: new WrapContentSize(),
                margins: {
                    top: 20,
                },
                text: new Template('temp'),
            }),
        ],
    }),
    weatherDayCard: new ContainerBlock({
        orientation: 'vertical',
        width: new FixedSize({ value: 278 }),
        height: new WrapContentSize(),
        content_alignment_horizontal: 'center',
        items: [
            new TextBlock({
                ...text40m,
                text: new Template('weekDay'),
                width: new WrapContentSize(),
            }),
            new TextBlock({
                ...text40m,
                text_color: colorWhiteOpacity40,
                text: new Template('date'),
                width: new WrapContentSize(),
            }),
            new ImageBlock({
                image_url: new Template('icon'),
                width: new FixedSize({ value: 148 }),
                height: new FixedSize({ value: 148 }),
                margins: {
                    top: 32,
                },
            }),
            new ContainerBlock({
                orientation: 'horizontal',
                width: new WrapContentSize(),
                height: new WrapContentSize(),
                margins: {
                    top: 32,
                },
                content_alignment_vertical: 'bottom',
                items: [
                    new TextBlock({
                        ...title56m,
                        text: new Template('dayTemp'),
                        width: new WrapContentSize(),
                    }),
                    new TextBlock({
                        ...title56m,
                        text_color: colorWhiteOpacity50,
                        text: new Template('nightTemp'),
                        width: new WrapContentSize(),
                    }),
                ],
            }),
        ],
    }),
    weekendDay: new ContainerBlock({
        width: new MatchParentSize({ weight: 1 }),
        height: new MatchParentSize(),
        items: [
            new ContainerBlock({
                width: new WrapContentSize(),
                height: new WrapContentSize(),
                orientation: 'horizontal',
                content_alignment_horizontal: 'left',
                items: [
                    new TextBlock({
                        ...text40m,
                        text: new Template('weekday'),
                        width: new WrapContentSize(),
                        height: new WrapContentSize(),
                    }),
                    new TextBlock({
                        ...text40m,
                        text_color: colorWhiteOpacity50,
                        text: new Template('date'),
                        width: new WrapContentSize(),
                        height: new WrapContentSize(),
                    }),
                ],
            }),
            new ContainerBlock({
                width: new WrapContentSize(),
                height: new WrapContentSize(),
                margins: {
                    top: 24,
                },
                orientation: 'horizontal',
                content_alignment_vertical: 'bottom',
                items: [
                    new ImageBlock({
                        image_url: new Template('icon'),
                        width: new FixedSize({ value: 132 }),
                        height: new FixedSize({ value: 132 }),
                    }),
                    new TextBlock({
                        ...title90m,
                        text: new Template('temp_day'),
                        width: new WrapContentSize(),
                        height: new WrapContentSize(),
                    }),
                    new TextBlock({
                        ...title64m,
                        text_color: colorWhiteOpacity50,
                        text: new Template('temp_night'),
                        margins: {
                            bottom: 8,
                        },
                        width: new WrapContentSize(),
                        height: new WrapContentSize(),
                    }),
                ],
            }),
        ],
    }),
};

export const tHelper = templateHelper(weatherTemplates);

type ContainerProps = {
    items: Div[];
    daypart: string;
    condition: string;
};
export const container = ({ items, daypart, condition }: ContainerProps) =>
    new ContainerBlock({
        width: new MatchParentSize(),
        height: new MatchParentSize(),
        background: [
            new ImageBackground({ image_url: getWeatherImage(daypart, condition) }),
        ],
        orientation: 'vertical',
        items,
    });

type NowBlockProps = {
    cityFormed: string;
    temp: number;
    iconType: string;
    feelsLike: number;
    title: string;
    when: string;
};
export const nowBlock = ({ cityFormed, temp, iconType, feelsLike, title, when }: NowBlockProps) =>
    new ContainerBlock({
        height: new WrapContentSize(),
        paddings: {
            left: 48,
            top: 48,
            right: 48,
        },
        items: [
            new TextBlock({
                ...text40m,
                text_color: colorWhiteOpacity50,
                width: new WrapContentSize(),
                height: new WrapContentSize(),
                text: when + ' ' + cityFormed,
            }),
            new ContainerBlock({
                orientation: 'horizontal',
                content_alignment_vertical: 'center',
                height: new WrapContentSize(),
                items: [
                    new TextBlock({
                        ...title146m,
                        width: new WrapContentSize(),
                        text: formatTemp(temp),
                    }),
                    new ImageBlock({
                        width: new FixedSize({ value: 176 }),
                        height: new FixedSize({ value: 176 }),
                        margins: {
                            top: 8,
                            left: 16,
                        },
                        image_url: formWeatherIconUrl(iconType ?? '', 176),
                    }),
                    new ContainerBlock({
                        width: new WrapContentSize(),
                        height: new WrapContentSize(),
                        orientation: 'vertical',
                        margins: {
                            left: 36,
                        },
                        items: [
                            new TextBlock({
                                ...text40m,
                                text_color: colorWhiteOpacity50,
                                width: new WrapContentSize(),
                                text: 'Ощущается как ' + formatTemp(feelsLike),
                            }),
                            new TextBlock({
                                ...text40m,
                                text_color: colorWhiteOpacity50,
                                width: new WrapContentSize(),
                                text: title,
                                margins: {
                                    top: 8,
                                },
                            }),
                        ],
                    }),
                ],
            }),
        ],
    });

type HoursGalleryProps = {
    hours: NAlice.NData.ITWeatherHourItem[];
};
export const hoursGallery = ({ hours }: HoursGalleryProps) =>
    new GalleryBlock({
        width: new MatchParentSize(),
        height: new WrapContentSize(),
        paddings: {
            left: 24,
            right: 48,
            top: 48,
        },
        orientation: 'horizontal',
        item_spacing: 24,
        items: hours.map(({ Hour, Icon, Temperature }) =>
            tHelper.weatherCard({
                time: formatHour(Hour ?? 0),
                icon: Icon ?? '',
                temp: formatTemp(Temperature ?? 0),
            }),
        ),
    });

export const dayParts = (parts: NAlice.NData.ITWeatherDayPartData[]) =>
    new GalleryBlock({
        orientation: 'horizontal',
        width: new WrapContentSize(),
        height: new WrapContentSize(),
        alignment_horizontal: 'center',
        paddings: {
            left: 48,
            right: 48,
        },
        item_spacing: 28,
        items: parts.map(({ DayPartType, Icon, Temperature }) =>
            tHelper.weatherCard({
                time: capitalize(dayPartsForms[DayPartType ?? 'day']),
                icon: Icon ?? '',
                temp: formatTemp(Temperature ?? 0),
            }),
        ),
    });

export const isWeekend = (
    days: Void<NAlice.NData.ITWeatherDayItem[]>,
): days is [NAlice.NData.ITWeatherDayItem, NAlice.NData.ITWeatherDayItem] =>
    days?.length === 2 && days[0].WeekDay === 6 && days[1].WeekDay === 0;

export const dateRange = (days: NAlice.NData.ITWeatherDayItem[]) => {
    switch (true) {
        case days.length === 7:
            return 'на неделю';
        case isWeekend(days):
            return 'на выходные';
    }

    return `на ${days.length} ${pluralize(days.length, { one: 'день', some: 'дня', many: 'дней' })}`;
};

export const daysGallery = (days: NAlice.NData.ITWeatherDayItem[]) =>
    new GalleryBlock({
        width: new MatchParentSize(),
        height: new WrapContentSize(),
        margins: {
            top: 80,
            bottom: 48,
        },
        paddings: {
            left: 48,
            right: 48,
        },
        orientation: 'horizontal',
        item_spacing: 24,
        items: days.map(({ Date: d, DayTemp, NightTemp, IconType }) =>
            tHelper.weatherDayCard({
                date: formatDate(new Date(d ?? 0), 'd MMMM'),
                weekDay: capitalize(formatDate(new Date(d ?? 0), 'cccc')),
                dayTemp: formatTemp(DayTemp ?? 0) + ' ',
                nightTemp: ' ' + formatTemp(NightTemp ?? 0),
                icon: formWeatherIconUrl(IconType ?? '', 176),
            }),
        ),
    });

export const weekendCast = (days: [NAlice.NData.ITWeatherDayItem, NAlice.NData.ITWeatherDayItem]) => {
    const divDays: Div[] = days.map(({ Date: VDate, IconType, DayTemp, NightTemp }) => tHelper.weekendDay({
        date: formatDate(new Date(VDate ?? 0), 'd MMMM'),
        weekday: capitalize(formatDate(new Date(VDate ?? 0), 'cccc')) + ' ',
        icon: formWeatherIconUrl(IconType ?? '', 132),
        temp_day: ' ' + formatTemp(DayTemp ?? 0) + ' ',
        temp_night: formatTemp(NightTemp ?? 0),
    }));
    divDays.splice(1, 0, new ContainerBlock({
        width: new FixedSize({ value: 1 }),
        height: new MatchParentSize(),
        margins: {
            left: 48,
            right: 48,
        },
        background: [
            new SolidBackground({ color: colorWhiteOpacity53 }),
        ],
        items: [new TextBlock({ text: ' ' })],
    }));

    return new ContainerBlock({
        width: new MatchParentSize(),
        height: new FixedSize({ value: 320 }),
        margins: {
            top: 170,
            bottom: 48,
        },
        paddings: {
            left: 48,
            right: 48,
        },
        orientation: 'horizontal',
        content_alignment_horizontal: 'center',
        items: divDays,
    });
};

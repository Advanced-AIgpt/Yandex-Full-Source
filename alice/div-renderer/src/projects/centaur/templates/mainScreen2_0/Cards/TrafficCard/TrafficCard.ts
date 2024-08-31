import {
    ContainerBlock,
    Div,
    FixedSize,
    ImageBlock,
    TextBlock,
    WrapContentSize,
    SolidBackground,
    MatchParentSize,
} from 'divcard2';
import { compact } from 'lodash';
import {
    ITrafficCardProps,
    ITrafficForecast,
} from './types';
import {
    text28m,
    title88m,
} from '../../../../style/Text/Text';
import {
    colorWhite,
    colorWhiteOpacity50,
} from '../../../../style/constants';
import { BasicTextCard } from '../BasicTextCard';
import BasicErrorCard from '../BasicErrorCard';
import { logger } from '../../../../../../common/logger';
import { getS3Asset } from '../../../../helpers/assets';
import EmptyDiv from '../../../../components/EmptyDiv';

import {
    CURRENT_TRAFFIC_VALUE_SIZE,
    CURRENT_TRAFFIC_SPACE_BETWEEN_ELEMENTS,
    TRAFFIC_FORECAST_BORDER_RADIUS,
    TRAFFIC_FORECAST_ITEM_HEIGHT, TRAFFIC_COLOR_SET, colorTrafficNA, TRAFFIC_FORECAST_ITEM_WIDTH,
} from './constants';
import getColorSet, { IColorSet } from '../../../../style/colorSet';
import { getCardMainScreenId } from '../helpers';

function getStateColor(trafficScore: number | null | undefined) : string {
    return (
        trafficScore !== null && typeof trafficScore !== 'undefined' && TRAFFIC_COLOR_SET[trafficScore][0]
    ) || colorTrafficNA;
}

function CurrentTraffic({
    text,
    trafficValue,
}: {
    text?: string;
    trafficValue: number;
}): Div {
    return new ContainerBlock({
        orientation: 'horizontal',
        content_alignment_vertical: 'center',
        content_alignment_horizontal: 'left',
        items: compact([
            new TextBlock({
                ...title88m,
                text_color: colorWhite,
                width: new WrapContentSize(),
                text: `${trafficValue}`,
            }),
            TrafficLights({
                size: CURRENT_TRAFFIC_VALUE_SIZE,
                color: getStateColor(trafficValue),
            }),
            text && new TextBlock({
                ...text28m,
                margins: {
                    left: CURRENT_TRAFFIC_SPACE_BETWEEN_ELEMENTS,
                },
                text_color: colorWhiteOpacity50,
                text_alignment_horizontal: 'left',
                width: new MatchParentSize(),
                text,
            }),
        ]),
    });
}

function TrafficLights({
    color,
    size,
}: {
    color: string;
    size: number;
}) {
    return new ContainerBlock({
        orientation: 'overlap',
        content_alignment_vertical: 'top',
        content_alignment_horizontal: 'center',
        width: new FixedSize({ value: size }),
        height: new FixedSize({ value: size }),
        margins: {
            left: CURRENT_TRAFFIC_SPACE_BETWEEN_ELEMENTS,
        },
        items: [
            new ContainerBlock({
                orientation: 'horizontal',
                content_alignment_vertical: 'center',
                content_alignment_horizontal: 'center',
                margins: {
                    top: 1,
                },
                items: [
                    new EmptyDiv({
                        width: new FixedSize({ value: size * 0.06 }),
                    }),
                    new ContainerBlock({
                        width: new FixedSize({ value: size * 0.94 }),
                        height: new FixedSize({ value: size * 0.94 }),
                        border: {
                            corner_radius: Math.floor(size / 2),
                        },
                        background: [
                            new SolidBackground({ color }),
                        ],
                        items: [],
                    }),
                ],
            }),
            new ImageBlock({
                image_url: getS3Asset('main-screen/cards/jams/traffic_lights_icon.png'),
                alignment_vertical: 'top',
                alignment_horizontal: 'right',
                preload_required: 1,
            }),
        ],
    });
}

function getTrafficColors(value: number | null | undefined): Readonly<[string, string]> {
    if (value === null || typeof value === 'undefined' || typeof TRAFFIC_COLOR_SET[value] === 'undefined') {
        return [colorTrafficNA, colorWhite];
    }
    return TRAFFIC_COLOR_SET[value];
}

function TrafficForecastItem({ time, trafficValue }: ITrafficForecast, colorSet: IColorSet) {
    const [bgColor, color] = getTrafficColors(trafficValue);

    return new ContainerBlock({
        orientation: 'vertical',
        width: new WrapContentSize(),
        content_alignment_vertical: 'center',
        content_alignment_horizontal: 'center',
        items: [
            new TextBlock({
                ...text28m,
                width: new FixedSize({ value: TRAFFIC_FORECAST_ITEM_WIDTH }),
                height: new FixedSize({ value: TRAFFIC_FORECAST_ITEM_HEIGHT }),
                text_alignment_horizontal: 'center',
                text_alignment_vertical: 'center',
                text_color: color,
                border: {
                    corner_radius: TRAFFIC_FORECAST_BORDER_RADIUS,
                },
                background: [
                    new SolidBackground({ color: bgColor }),
                ],
                text: `${trafficValue}`,
            }),
            new TextBlock({
                ...text28m,
                margins: {
                    top: 4,
                },
                width: new MatchParentSize(),
                height: new FixedSize({ value: TRAFFIC_FORECAST_ITEM_HEIGHT }),
                text_alignment_horizontal: 'center',
                text_color: colorSet.textColorOpacity50,
                text: `${time}`,
            }),
        ],
    });
}

function TrafficForecast({ forecasts, colorSet }: {
    forecasts: ITrafficForecast[];
    colorSet: IColorSet;
}): Div {
    const forecastItems: Div[] = [];

    for (let i = 0; i < forecasts.length; i++) {
        forecastItems.push(TrafficForecastItem(forecasts[i], colorSet));
        if (i + 1 < forecasts.length) {
            forecastItems.push(new EmptyDiv({
                width: new MatchParentSize({ weight: 1 }),
            }));
        }
    }

    return new ContainerBlock({
        orientation: 'horizontal',
        content_alignment_vertical: 'center',
        content_alignment_horizontal: 'left',
        width: new MatchParentSize(),
        items: forecastItems,
    });
}

export default function TrafficCard({
    trafficValue,
    forecasts,
    actions,
    longtap_actions,
    city,
    rowIndex,
    colIndex,
}: Partial<ITrafficCardProps>): Div {
    const colorSet = getColorSet();

    if (trafficValue === null || typeof trafficValue === 'undefined' || !forecasts) {
        logger.error('The traffic card should have a text, trafficValue, trafficColor and forecasts, but some of this was not transmitted');

        return BasicErrorCard({
            colIndex,
            rowIndex,
            title: 'Пробки',
            description: 'Ошибка сети :(\nНе могу показать уровень пробок. Проверьте интернет.',
            actions,
            longtap_actions,
        });
    }

    const cardData: Parameters<typeof BasicTextCard>[0] = {
        id: getCardMainScreenId({ colIndex, rowIndex }),
        title: city || 'Пробки',
        items: [
            CurrentTraffic({ trafficValue }),
            new EmptyDiv({
                height: new MatchParentSize(),
            }),
            TrafficForecast({ forecasts, colorSet }),
        ],
        actions,
        longtap_actions,
        colorSet,
    };

    return BasicTextCard(cardData);
}

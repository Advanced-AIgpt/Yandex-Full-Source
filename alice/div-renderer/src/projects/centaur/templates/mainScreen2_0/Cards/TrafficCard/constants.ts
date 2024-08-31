import { colorWhite } from '../../../../style/constants';

export const colorTrafficFree = '#66B616';
export const colorTrafficLight = '#FAE268';
export const colorTextTrafficLight = '#785C00';
export const colorTrafficHard = '#E55245';
export const colorTrafficNA = '#CCCCCC';
export const colorTrafficOff = '#FFFFFF';

export const CURRENT_TRAFFIC_VALUE_SIZE = 68;
export const CURRENT_TRAFFIC_SPACE_BETWEEN_ELEMENTS = 22;
export const TRAFFIC_FORECAST_ITEM_WIDTH = 52;
export const TRAFFIC_FORECAST_ITEM_HEIGHT = 36;
export const TRAFFIC_FORECAST_BORDER_RADIUS = Math.floor(
    Math.min(
        TRAFFIC_FORECAST_ITEM_WIDTH,
        TRAFFIC_FORECAST_ITEM_HEIGHT,
    ) / 2,
);

const TRAFFIC_COLOR_SET_FREE = [colorTrafficFree, colorWhite] as const;
const TRAFFIC_COLOR_SET_LIGHT = [colorTrafficLight, colorTextTrafficLight] as const;
const TRAFFIC_COLOR_SET_HARD = [colorTrafficHard, colorWhite] as const;

export const TRAFFIC_COLOR_SET: Readonly<Readonly<[string, string]>[]> = [
    TRAFFIC_COLOR_SET_FREE, //     0
    TRAFFIC_COLOR_SET_FREE, //     1
    TRAFFIC_COLOR_SET_FREE, //     2
    TRAFFIC_COLOR_SET_FREE, //     3
    TRAFFIC_COLOR_SET_LIGHT, //    4
    TRAFFIC_COLOR_SET_LIGHT, //    5
    TRAFFIC_COLOR_SET_HARD, //     6
    TRAFFIC_COLOR_SET_HARD, //     7
    TRAFFIC_COLOR_SET_HARD, //     8
    TRAFFIC_COLOR_SET_HARD, //     9
    TRAFFIC_COLOR_SET_HARD, //     10
];

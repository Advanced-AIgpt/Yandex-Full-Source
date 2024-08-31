import {
    ContainerBlock,
    DivCustomBlock,
    FixedSize,
    ImageBlock,
    MatchParentSize,
    SolidBackground,
    TextBlock,
    WrapContentSize,
} from 'divcard2';
import { compact } from 'lodash';
import { NAlice } from '../../../../protos';
import { formatTemp, formWeatherIconUrl } from '../weather/common';
import { colorWhiteOpacity0 } from '../../style/constants';
import { title164m, title36m } from '../../style/Text/Text';
import EmptyDiv from '../../components/EmptyDiv';
import { MMRequest } from '../../../../common/helpers/MMRequest';
import { IRequestState } from '../../../../common/types/common';
import { ExpFlags } from '../../expFlags';
import { DualScreen } from '../../components/DualScreen/DualScreen';
import { TopLevelCard } from '../../helpers/helpers';

function oldChromeDiv(WeatherTeaserData: NAlice.NData.ITWeatherTeaserData | null | undefined) {
    return new ContainerBlock({
        width: new MatchParentSize(),
        height: new MatchParentSize(),
        background: [
            new SolidBackground({ color: colorWhiteOpacity0 }),
        ],
        paddings: {
            top: 18,
            left: 48,
        },
        items: compact([
            ClockBlock(),
            WeatherTeaserData &&
            typeof WeatherTeaserData.Temperature !== 'undefined' &&
            WeatherTeaserData.Temperature !== null &&
            WeatherTeaserData.IconType &&
            WeatherBlock(WeatherTeaserData),
        ]),
    });
}

function ChromeDiv(WeatherTeaserData: NAlice.NData.ITWeatherTeaserData | null | undefined, requestState: IRequestState) {
    return DualScreen({
        firstDiv: [new ContainerBlock({
            width: new MatchParentSize({ weight: 1 }),
            height: new MatchParentSize({ weight: 1 }),
            content_alignment_horizontal: 'center',
            content_alignment_vertical: 'center',
            items: [
                new TextBlock({
                    ...title164m,
                    text: ' ',
                    text_alignment_horizontal: 'center',
                    extensions: [
                        {
                            id: 'clock_time',
                            params: {
                                pattern: 'HH:mm',
                            },
                        },
                    ],
                }),
                new ContainerBlock({
                    orientation: 'horizontal',
                    content_alignment_horizontal: 'center',
                    content_alignment_vertical: 'center',
                    items: compact([
                        new TextBlock({
                            ...title36m,
                            text: ' ',
                            width: new WrapContentSize(),
                            margins: {
                                right: 16,
                            },
                            extensions: [
                                {
                                    id: 'clock_time',
                                    params: {
                                        pattern: 'EE, d MMMM',
                                    },
                                },
                            ],
                        }),
                        WeatherTeaserData &&
                        typeof WeatherTeaserData.Temperature !== 'undefined' &&
                        WeatherTeaserData.Temperature !== null &&
                        WeatherTeaserData.IconType &&
                        WeatherBlock(WeatherTeaserData),
                    ]),
                }),
            ],
        })],
        secondDiv: [new EmptyDiv()],
        requestState,
        mainColor1: colorWhiteOpacity0,
        mainColor: colorWhiteOpacity0,
    });
}

const WeatherBlock = (
    WeatherTeaserData: NAlice.NData.ITWeatherTeaserData,
): ContainerBlock => {
    return new ContainerBlock({
        width: new WrapContentSize(),
        height: new WrapContentSize(),
        orientation: 'horizontal',
        content_alignment_vertical: 'center',
        items: [
            new ImageBlock({
                image_url: formWeatherIconUrl(WeatherTeaserData?.IconType ?? '', 64),
                width: new FixedSize({ value: 52 }),
                height: new FixedSize({ value: 52 }),
                preload_required: 1,
            }),
            new TextBlock({
                ...title36m,
                text: ' ' + formatTemp(WeatherTeaserData?.Temperature ?? 0),
            }),
        ],
    });
};

const ClockBlock = () => {
    return new DivCustomBlock({
        custom_type: 'centaur_time',
        height: new WrapContentSize(),
        width: new WrapContentSize(),
    });
};

export const TeaserChrome = (
    { WeatherTeaserData }: NAlice.NData.ITCentaurTeaserChromeDefaultLayerData,
    _: MMRequest,
    requestState: IRequestState,
) => {
    const chromeDiv = requestState.hasExperiment(ExpFlags.extendedNewsDesignWithDoubleScreen2)
        ? ChromeDiv
        : oldChromeDiv;

    return TopLevelCard( {
        log_id: 'centaur.teaser.chrome',
        states: [
            {
                state_id: 0,
                div: chromeDiv(WeatherTeaserData, requestState),
            },
        ],
    }, requestState);
};

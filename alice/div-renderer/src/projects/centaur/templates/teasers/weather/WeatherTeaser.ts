import {
    ContainerBlock,
    FixedSize,
    ImageBackground,
    ImageBlock,
    MatchParentSize,
    TemplateCard,
    Templates,
    TextBlock,
} from 'divcard2';
import { NAlice } from '../../../../../protos';
import {
    calcCondition,
    calcDaypart, formWeatherIconUrl,
    getWeatherImage,
    weatherTemplates,
} from '../../weather/common';
import { DualScreen } from '../../../components/DualScreen/DualScreen';
import { title240m, title56r } from '../../../style/Text/Text';
import { offsetFromEdgeOfPartOfScreen, scalableValue } from '../../../style/constants';
import getColorSet from '../../../style/colorSet';
import { MMRequest } from '../../../../../common/helpers/MMRequest';
import { IRequestState } from '../../../../../common/types/common';

interface IDualWeatherTeaserProps {
    title: string;
    temperature: number;
    comment: string;
    bgImage: string;
    iconImage: string;
    requestState: IRequestState;
}

export function DualWeatherTeaser({
    title,
    temperature,
    comment,
    bgImage,
    iconImage,
    requestState,
}: IDualWeatherTeaserProps) {
    const theme = getColorSet({
        id: title,
    });

    return DualScreen({
        mainColor1: theme.mainColor1,
        mainColor: theme.mainColor,
        requestState,
        firstDiv: [new ContainerBlock({
            width: new MatchParentSize({ weight: 1 }),
            height: new MatchParentSize({ weight: 1 }),
            content_alignment_vertical: 'center',
            content_alignment_horizontal: 'center',
            paddings: {
                top: offsetFromEdgeOfPartOfScreen,
                left: offsetFromEdgeOfPartOfScreen,
                bottom: offsetFromEdgeOfPartOfScreen,
                right: offsetFromEdgeOfPartOfScreen,
            },
            items: [
                new TextBlock({
                    ...title56r,
                    text: title,
                    text_alignment_horizontal: 'center',
                }),
                new TextBlock({
                    ...title240m,
                    text: `${temperature}°`,
                    text_alignment_horizontal: 'center',
                }),
                new TextBlock({
                    ...title56r,
                    text: comment,
                    text_alignment_horizontal: 'center',
                }),
            ],
        })],
        secondDiv: [new ContainerBlock({
            width: new MatchParentSize({ weight: 1 }),
            height: new MatchParentSize({ weight: 1 }),
            content_alignment_horizontal: 'center',
            content_alignment_vertical: 'center',
            background: [new ImageBackground({
                image_url: bgImage,
            })],
            items: [
                new ImageBlock({
                    image_url: formWeatherIconUrl(iconImage, 132),
                    width: new FixedSize({ value: scalableValue(526) }),
                    height: new FixedSize({ value: scalableValue(526) }),
                }),
            ],
        })],
    });
}

export const WeatherTeaser = ({
    Sunset,
    Sunrise,
    UserTime,
    Condition,
    GeoLocation,
    Temperature,
    IconType,
}: NAlice.NData.ITWeatherTeaserData, _:MMRequest, requestState: IRequestState) => {
    const daypart = calcDaypart(Sunrise, Sunset, UserTime);
    const condition = calcCondition(Condition?.Cloudness ?? 0, Condition?.PrecStrength ?? 0);
    const wi = getWeatherImage(daypart, condition);
    return new TemplateCard(new Templates(weatherTemplates), {
        log_id: 'weather.teasers',
        states: [
            {
                state_id: 0,
                div: DualWeatherTeaser({
                    requestState,
                    bgImage: wi,
                    comment: Condition?.FeelsLike ? `Ощущается как ${Condition.FeelsLike}°` : '',
                    temperature: Temperature ?? 0,
                    title: `Сейчас в городе ${GeoLocation?.City || ''}`,
                    iconImage: IconType || '',
                }),
            },
        ],
        variable_triggers: requestState.variableTriggers.getAll(),
    });
};

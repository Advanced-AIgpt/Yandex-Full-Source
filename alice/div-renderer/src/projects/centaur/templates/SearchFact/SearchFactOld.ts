import {
    ContainerBlock,
    FixedSize,
    ImageBlock,
    MatchParentSize,
    TemplateCard,
    Templates,
    TextBlock,
    WrapContentSize,
} from 'divcard2';
import { compact, upperFirst } from 'lodash';
import { NAlice } from '../../../../protos';
import { SuggestsBlock } from '../suggests';
import { getAsrTextFromInput, MMRequest } from '../../../../common/helpers/MMRequest';
import {
    colorWhiteOpacity50,
    colorWhiteOpacity90,
    offsetFromEdgeOfScreen,
    offsetTopWithSearchItem, simpleBackground,
} from '../../style/constants';
import { sourceHost } from '../../components/sourceHost';
import {
    text40m,
    text40r,
    title32m,
    title44r,
    title48m,
    title56m,
    title60m,
    title64m,
    title88m,
} from '../../style/Text/Text';
import EmptyDiv from '../../components/EmptyDiv';
import { CloseButtonWrapper } from '../../components/CloseButtonWrapper/CloseButtonWrapper';
import { textBreakpoint } from '../../helpers/helpers';

export const wrapFactsText = textBreakpoint(
    [90, title64m],
    [130, title60m],
    [175, title56m],
    [240, title48m],
    [280, title44r],
    [Number.POSITIVE_INFINITY, text40r],
);

const header = (t: string) => {
    return new TextBlock({
        ...text40m,
        text_color: colorWhiteOpacity50,
        text: upperFirst(t),
        height: new WrapContentSize(),
        width: new WrapContentSize(),
    });
};

const answer = (t: string) => {
    return new TextBlock({
        margins: {
            top: 24,
        },
        font_weight: 'medium',
        text_color: colorWhiteOpacity90,
        height: new WrapContentSize(),
        width: new WrapContentSize(),
        ...wrapFactsText(upperFirst(t)),
    });
};

const kkalAnswer = (t: string) => {
    return new TextBlock({
        ...title88m,
        text_color: colorWhiteOpacity90,
        height: new WrapContentSize(),
        width: new WrapContentSize(),
        text: t,
    });
};

const image = (url: string) => {
    return new ImageBlock({
        image_url: url,
        width: new FixedSize({ value: 120 }),
        height: new FixedSize({ value: 120 }),
        border: { corner_radius: 90 },
    });
};

const title = (t: string) => {
    return new TextBlock({
        ...title32m,
        text: t,
        margins: {
            right: 16,
        },
        height: new WrapContentSize(),
        width: new WrapContentSize(),
    });
};

const simpleFact = (
    {
        Question,
        Text,
        Title,
        Hostname,
    }: NAlice.NData.ITSearchFactData,
    mmRequest: MMRequest,
) => {
    const TextRequest = getAsrTextFromInput(mmRequest) ?? Question;
    return {
        items: compact([
            new EmptyDiv({
                width: new MatchParentSize(),
                height: new MatchParentSize({
                    weight: 1,
                }),
            }),
            new ContainerBlock({
                paddings: {
                    left: offsetFromEdgeOfScreen,
                    right: offsetFromEdgeOfScreen,
                },
                margins: {
                    bottom: 20,
                },
                items: compact([
                    TextRequest && header(TextRequest),
                    Text && answer(Text),
                    (Title || Hostname) &&
                    new ContainerBlock({
                        orientation: 'horizontal',
                        content_alignment_vertical: 'bottom',
                        margins: {
                            top: 16,
                        },
                        items: compact([
                            Title && title(Title),
                            Hostname && sourceHost(Hostname),
                        ]),
                    }),
                ]),
            }),
            new EmptyDiv({
                width: new MatchParentSize(),
                height: new MatchParentSize({
                    weight: 1,
                }),
            }),
            SuggestsBlock(mmRequest.ScenarioResponseBody, {
                paddings: {
                    left: offsetFromEdgeOfScreen,
                    right: offsetFromEdgeOfScreen,
                },
            }),
        ]),
    };
};

type factComponent = (
    searchFactData: NAlice.NData.ITSearchFactData,
    mmRequest: MMRequest
) => Partial<ConstructorParameters<typeof ContainerBlock>[0]>;

const extendedFactNutritionalValue: factComponent = (
    {
        Text,
        Hostname,
        Image,
        SerpData,
    }: NAlice.NData.ITSearchFactData,
    mmRequest: MMRequest,
) => {
    const infoString = SerpData?.Question && `${SerpData?.Question} - Пищевая ценность`;
    return {
        items: compact([
            new ContainerBlock({
                orientation: 'horizontal',
                items: compact([
                    Image && image(Image),
                    new TextBlock({
                        text: ' ',
                        width: new FixedSize({ value: 24 }),
                    }),
                    new ContainerBlock({
                        orientation: 'horizontal',
                        items: compact([
                            Text && kkalAnswer(Text),
                            SuggestsBlock(mmRequest.ScenarioResponseBody),
                        ]),
                    }),
                ]),
            }),
            infoString && new TextBlock({
                ...text40r,
                text: infoString,
                margins: {
                    top: 22,
                },
            }),
            Hostname && sourceHost(Hostname, {
                margins: {
                    top: infoString ? 16 : 22,
                },
            }),
        ]),
        paddings: {
            left: offsetFromEdgeOfScreen,
            right: offsetFromEdgeOfScreen,
            top: offsetTopWithSearchItem,
            bottom: offsetFromEdgeOfScreen,
        },
        content_alignment_vertical: 'center',
    };
};

const factsTypes: { [name: string]: factComponent } = {
    calories_fact: extendedFactNutritionalValue,
};

const fact = (
    searchFactData: NAlice.NData.ITSearchFactData,
    mmRequest: MMRequest,
) => {
    const factComponent = searchFactData.SnippetType && typeof factsTypes[searchFactData.SnippetType] !== 'undefined' ?
        factsTypes[searchFactData.SnippetType] : simpleFact;
    return factComponent(
        searchFactData,
        mmRequest,
    );
};

export default function SearchFactOld(
    searchFactData: NAlice.NData.ITSearchFactData,
    mmRequest: MMRequest,
) {
    return new TemplateCard(new Templates({}), {
        log_id: 'fact_card',
        states: [
            {
                state_id: 0,
                div: CloseButtonWrapper({
                    div: new ContainerBlock({
                        background: simpleBackground,
                        height: new MatchParentSize(),
                        width: new MatchParentSize(),
                        paddings: {
                            top: offsetTopWithSearchItem,
                            bottom: offsetFromEdgeOfScreen,
                        },
                        ...fact(searchFactData, mmRequest),
                    } as ConstructorParameters<typeof ContainerBlock>[0]),
                }),
            },
        ],
    });
}

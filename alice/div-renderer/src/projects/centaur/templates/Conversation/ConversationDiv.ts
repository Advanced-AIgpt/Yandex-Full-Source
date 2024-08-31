import {
    ContainerBlock,
    FixedSize,
    GalleryBlock,
    GradientBackground,
    MatchParentSize,
    SolidBackground,
    TextBlock,
    WrapContentSize,
} from 'divcard2';
import FullScreen from '../../components/FullScreen';
import { boltalkaColorSet, IColorSet } from '../../style/colorSet';
import { text40r, title36m, title44r, title48m, title56m, title60m, title64m } from '../../style/Text/Text';
import { IRequestState } from '../../../../common/types/common';
import EmptyDiv from '../../components/EmptyDiv';
import { textBreakpoint } from '../../helpers/helpers';
import { SuggestsBlock } from '../suggests';
import { NAlice } from '../../../../protos';
import { offsetFromEdgeOfScreen } from '../../style/constants';
import { CloseButtonWrapper } from '../../components/CloseButtonWrapper/CloseButtonWrapper';
import { Layer } from '../../common/layers';
type ITScenarioResponseBody = NAlice.NScenarios.ITScenarioResponseBody;

interface IConversationDivProps {
    requestText: string;
    responseText: string;
    colorSet: IColorSet;
    response: ITScenarioResponseBody;
}

const conversationTextStyle = textBreakpoint(
    [90, title64m],
    [130, title60m],
    [150, title56m],
    [215, title48m],
    [250, title44r],
    [Number.POSITIVE_INFINITY, text40r],
);

export default function ConversationDiv({
    requestText,
    responseText,
    colorSet,
    response,
}: IConversationDivProps, requestState: IRequestState) {
    return CloseButtonWrapper({
        div: FullScreen({
            children: [
                new ContainerBlock({
                    width: new MatchParentSize({ weight: 1 }),
                    height: new MatchParentSize({ weight: 1 }),
                    items: [
                        new EmptyDiv({
                            height: new MatchParentSize({ weight: 1 }),
                        }),
                        new GalleryBlock({
                            height: new WrapContentSize(),
                            orientation: 'vertical',
                            paddings: {
                                top: 168,
                                left: 168,
                                bottom: 168,
                                right: 168,
                            },
                            items: [
                                new TextBlock({
                                    ...title36m,
                                    text: requestText,
                                    text_color: colorSet.textColorOpacity50,
                                    width: new MatchParentSize({ weight: 1 }),
                                    height: new WrapContentSize(),
                                    text_alignment_horizontal: 'center',
                                    max_lines: 2,
                                    margins: {
                                        bottom: 16,
                                    },
                                }),
                                new TextBlock({
                                    ...conversationTextStyle(responseText),
                                    text: responseText,
                                    text_color: colorSet.textColor,
                                    width: new MatchParentSize({ weight: 1 }),
                                    height: new WrapContentSize(),
                                    text_alignment_horizontal: 'center',
                                }),
                            ],
                        }),
                        new EmptyDiv({
                            height: new MatchParentSize({ weight: 1 }),
                        }),
                    ],
                }),
                new EmptyDiv({
                    height: new FixedSize({ value: 168 }),
                    background: [
                        new GradientBackground({
                            angle: 90,
                            colors: [colorSet.mainColorOpacity0, colorSet.mainColorOpacity90],
                        }),
                    ],
                }),
                SuggestsBlock(response, {
                    alignment_vertical: 'bottom',
                    paddings: {
                        top: offsetFromEdgeOfScreen,
                        left: offsetFromEdgeOfScreen,
                        bottom: offsetFromEdgeOfScreen,
                        right: offsetFromEdgeOfScreen,
                    },
                    background: [
                        new GradientBackground({
                            angle: 90,
                            colors: [colorSet.mainColorOpacity60, colorSet.mainColorOpacity0],
                        }),
                    ],
                }, {
                    background: [new SolidBackground({ color: colorSet.suggestsBackground })],
                }),
            ],
            options: {
                background: [new SolidBackground({ color: boltalkaColorSet.mainColor })],
                orientation: 'overlap',
                height: new FixedSize({ value: requestState.sizes.height }),
            },
        }),
        layer: Layer.DIALOG,
        closeButtonProps: {
            options: {
                background: [],
            },
        },
    });
}

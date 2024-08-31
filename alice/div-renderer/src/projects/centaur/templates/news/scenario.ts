import {
    ContainerBlock,
    DivStateBlock,
    FixedSize,
    IDivAction,
    IDivAnimation,
    IDivStateBlockState,
    IndicatorBlock,
    MatchParentSize,
    PagerBlock,
    PageSize,
    PercentageSize, SolidBackground,
    WrapContentSize,
} from 'divcard2';
import { NewsCardProps, newsCommonCard } from './common';
import CloseButton from '../../components/CloseButton';
import { colorWhite, colorWhiteOpacity10, mainBackground, offsetFromEdgeOfScreen } from '../../style/constants';
import { NAlice } from '../../../../protos';
import { CLOSE_BUTTON_PADDING, renderExtendedNewsContent } from './extended';
import { MMRequest } from '../../../../common/helpers/MMRequest';
import { ExpFlags, hasExperiment } from '../../expFlags';
import { NewsExtended } from '../NewsExtended/NewsExtended';
import { setStateAction } from '../../../../common/actions/div';
import { IRequestState } from '../../../../common/types/common';
import { TopLevelCard } from '../../helpers/helpers';

export default function renderNews(
    { NewsItems, CurrentNewsItem, Topic, Tz }: NAlice.NData.ITNewsGalleryData,
    mmRequest: MMRequest,
    requestState: IRequestState,
) {
    const doubleExtendedNews = hasExperiment(mmRequest, ExpFlags.extendedNewsDesignWithDoubleScreen);

    const divCard = {
        log_id: 'news_scenario',
        states: [
            {
                state_id: 0,
                div: new DivStateBlock({
                    div_id: 'news',
                    default_state_id: 'pager',
                    height: new MatchParentSize(),
                    width: new MatchParentSize(),
                    states: [
                        {
                            state_id: 'pager',
                            div: newsPagerBlock({ NewsItems, CurrentNewsItem, Topic, Tz }),
                            animation_out: {
                                name: 'scale',
                                duration: 200,
                                start_value: 1,
                                end_value: 0.95,
                                interpolator: 'ease_out',
                            },
                            animation_in: {
                                name: 'scale',
                                duration: 200,
                                start_value: 0.9,
                                end_value: 1,
                                interpolator: 'ease_in_out',
                            },
                        },
                        ...(NewsItems?.map((item, newsItemId) => extendedNewsState({ item, topic: Topic ?? '', bottomGap: true, tz: Tz ?? '' }, newsItemId, requestState, doubleExtendedNews)) ?? []),
                    ],
                }),
            },
        ],
    };

    return TopLevelCard(
        divCard,
        requestState,
    );
}

const newsPagerBlock = ({ NewsItems, CurrentNewsItem, Topic, Tz }: NAlice.NData.ITNewsGalleryData) =>
    new ContainerBlock({
        width: new MatchParentSize(),
        height: new MatchParentSize(),
        orientation: 'overlap',
        items: [
            new PagerBlock({
                id: 'pager',
                width: new MatchParentSize(),
                height: new MatchParentSize(),
                layout_mode: new PageSize({ page_width: new PercentageSize({ value: 100 }) }),
                default_item: CurrentNewsItem ?? 0,
                items: NewsItems?.map((item, insidePagerNewsItemId) => newsPagerCard({ item, topic: Topic ?? '', bottomGap: true, tz: Tz ?? '' }, insidePagerNewsItemId)) ?? [],
            }),
            new ContainerBlock({
                height: new MatchParentSize(),
                width: new MatchParentSize(),
                content_alignment_horizontal: 'center',
                content_alignment_vertical: 'bottom',
                paddings: {
                    bottom: 48,
                },
                items: [
                    new IndicatorBlock({
                        height: new WrapContentSize(),
                        width: new MatchParentSize(),
                        pager_id: 'pager',
                        active_item_color: colorWhite,
                        inactive_item_color: colorWhiteOpacity10,
                        space_between_centers: new FixedSize({ value: 120 }),
                        shape: {
                            type: 'rounded_rectangle',
                            corner_radius: new FixedSize({ value: 20 }),
                            item_height: new FixedSize({ value: 8 }),
                            item_width: new FixedSize({ value: 90 }),
                        },
                    }),
                ],
            }),
            CloseButton({
                padding: CLOSE_BUTTON_PADDING,
                options: {
                    margins: {
                        top: offsetFromEdgeOfScreen,
                        right: offsetFromEdgeOfScreen,
                    },
                },
            }),
        ],
    });

const newsPagerCard = (data: NewsCardProps, newsItemId: number) =>
    newsCommonCard(data, openExtendedNewsDivAction(newsItemId));

function extendedNewsState(
    newsCardProps: NewsCardProps,
    newsItemId: number,
    requestState: IRequestState,
    doubleExtendedNews = false,
): IDivStateBlockState {
    return {
        state_id: `extended_${newsItemId}`,
        div: newsExtendedBlock(newsCardProps, requestState, doubleExtendedNews),
        animation_in: {
            ...commonAnimationProps,
            start_value: 1,
            end_value: 0,
            interpolator: 'ease_out',
        },
        animation_out: {
            ...commonAnimationProps,
            start_value: 0,
            end_value: 1,
            interpolator: 'ease_in',
        },
    };
}

const commonAnimationProps: IDivAnimation = {
    name: 'translate',
    duration: 350,
};

function openExtendedNewsDivAction(newsItemId: number): IDivAction {
    return {
        log_id: 'news_card.read.open.extended',
        url: setStateAction(`0/news/extended_${newsItemId}`, true),
    };
}

export function newsExtendedBlock(
    props: NewsCardProps,
    requestState: IRequestState,
    doubleExtendedNews = false,
) {
    const { item: { Text, Agency, Logo, PubDate, ExtendedNews }, topic, bottomGap, tz } = props;

    if (doubleExtendedNews) {
        return NewsExtended(props, requestState);
    }

    return new ContainerBlock({
        width: new MatchParentSize(),
        height: new MatchParentSize(),
        background: [new SolidBackground({ color: mainBackground })],
        orientation: 'overlap',
        paddings: {
            left: offsetFromEdgeOfScreen,
            right: offsetFromEdgeOfScreen,
        },
        items: [
            CloseButton({
                padding: CLOSE_BUTTON_PADDING,
                options: {
                    margins: {
                        top: offsetFromEdgeOfScreen,
                    },
                    actions: [
                        {
                            log_id: 'close_fullscreen',
                            url: setStateAction('0/news/pager', true),
                        },
                    ],
                },
                preventDefault: true,
            }),
            renderExtendedNewsContent({ item: { Text, Agency, Logo, PubDate, ExtendedNews }, topic, bottomGap, tz }),
        ],
    });
}

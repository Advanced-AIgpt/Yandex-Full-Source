import { ContainerBlock, IDivAction, MatchParentSize } from 'divcard2';
import { NAlice } from '../../../../protos';
import { offsetFromEdgeOfScreen } from '../../style/constants';
import { createShowViewClientAction } from '../../actions/client';
import { generateBackground, NewsCardProps, newsCommonCard } from './common';
import { renderExtendedNewsContent } from './extended';
import { MMRequest } from '../../../../common/helpers/MMRequest';
import { ExpFlags } from '../../expFlags';
import { NewsTeaser } from '../teasers/news/NewsTeaser';
import { directivesAction } from '../../../../common/actions';
import { IRequestState } from '../../../../common/types/common';
import { TopLevelCard } from '../../helpers/helpers';
import { createRequestState } from '../../../../registries/common';

export default function(data: NAlice.NData.ITNewsTeaserData, mmRequest: MMRequest, requestState: IRequestState) {
    if (requestState.hasExperiment(ExpFlags.teasersDesignWithDoubleScreenNews)) {
        return NewsTeaser(data, mmRequest, requestState);
    }

    const { NewsItem, Topic, Tz } = data;
    const newsCardProps = { item: NewsItem ?? {}, topic: Topic ?? '', bottomGap: false, tz: Tz ?? '' };
    return TopLevelCard({
        log_id: 'news_scenario',
        states: [
            {
                state_id: 0,
                div: {
                    div_id: 'root',
                    type: 'container',
                    width: new MatchParentSize(),
                    height: new MatchParentSize(),
                    items: [newsCommonCard(newsCardProps, openExtendedNewsDivAction(newsCardProps))],
                },
            },
        ],
    }, requestState);
}

function openExtendedNewsDivAction(card: NewsCardProps): IDivAction {
    return {
        log_id: 'news_card.read.open.extended',
        url: directivesAction(createShowViewClientAction(
            newsExtendedCard(card),
            false,
        )),
    };
}

function newsExtendedCard(
    newsCardProps: NewsCardProps,
) {
    const requestState = createRequestState();

    return TopLevelCard({
        log_id: 'extended.news.view',
        states: [
            {
                state_id: 0,
                div: newsExtendedBlock(newsCardProps),
            },
        ],
    }, requestState);
}

function newsExtendedBlock(
    { item: { Text, Image, Agency, Logo, PubDate, ExtendedNews }, topic, bottomGap, tz }: NewsCardProps,
) {
    return new ContainerBlock({
        width: new MatchParentSize(),
        height: new MatchParentSize(),
        background: generateBackground(topic ?? '', Image),
        orientation: 'overlap',
        paddings: {
            left: offsetFromEdgeOfScreen,
            right: offsetFromEdgeOfScreen,
        },
        items: [
            renderExtendedNewsContent({ item: { Text, Agency, Logo, PubDate, ExtendedNews }, topic, bottomGap, tz }),
        ],
    });
}

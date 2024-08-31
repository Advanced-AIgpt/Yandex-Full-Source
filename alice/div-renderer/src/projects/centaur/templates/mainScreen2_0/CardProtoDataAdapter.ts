import { NAlice } from '../../../../protos';
import { CardInfo } from './Cards/types';
import { IErrorCardProps } from './Cards/ErrorCard/types';
import { logger } from '../../../../common/logger';
import { getYouTubeCardData } from './Cards/YouTubeCard/getData';
import { getInfoCardData } from './Cards/InfoCard/getData';
import { getMusicCardData } from './Cards/MusicCard/getData';
import { getWeatherCardData } from './Cards/WeatherCard/getData';
import { getNewsCardData } from './Cards/NewsCard/getData';
import { getEmptyCardData } from './Cards/EmptyCard/getData';
import { getTrafficCardData } from './Cards/TrafficCard/getData';
import { getVideoCallLogoutedCardData, getVideoCallLoginedCardData } from './Cards/VideoCallCard/getData';
import { IRequestState } from '../../../../common/types/common';
import { getSkillCardData } from './Cards/SkillCard/getData';
import { directivesAction } from '../../../../common/actions';
import { createServerActionBasedOnTSF } from '../../../../common/actions/server';
import { isEmpty } from 'lodash';

type ITCardData = NAlice.NData.TCentaurMainScreenMyScreenData.TColumn.ITCardData;

function getErrorCardData(requestState: IRequestState): IErrorCardProps {
    return {
        type: 'error',
        requestState,
    };
}

export interface ICardDataOptions {
    colIndex?: number;
    rowIndex?: number;
    isChoice?: boolean;
}

export function CardProtoDataAdapter(
    card: ITCardData,
    requestState: IRequestState,
    options: ICardDataOptions = {},
): CardInfo {
    const { colIndex = 0, rowIndex = 0 } = options;
    const cardData = getInfoCardData(card, requestState) ||
        getSkillCardData(card, requestState) ||
        getMusicCardData(card, requestState) ||
        getYouTubeCardData(card, requestState) ||
        getNewsCardData(card, requestState) ||
        getWeatherCardData(card, requestState) ||
        getTrafficCardData(card, requestState) ||
        getVideoCallLoginedCardData(card, requestState, options) ||
        getVideoCallLogoutedCardData(card, requestState) ||
        getEmptyCardData(card, requestState);

    if (!cardData) {
        logger.error('Main screen card has unknown no type');
        return getErrorCardData(requestState);
    }

    if (card.TypedAction?.value && (typeof cardData.actions === 'undefined' || isEmpty(cardData.actions))) {
        const serverAction = createServerActionBasedOnTSF({
            binaryTsf: card.TypedAction?.value,
            productScenario: 'CentaurMainScreen',
            purpose: cardData.type + '_widget_action',
        });
        cardData.actions = serverAction ? [{
            log_id: 'main_screen_server_action',
            url: directivesAction(serverAction),
        }] : undefined;
    }

    if (card.LongTapTypedAction?.value && (typeof cardData.longtap_actions === 'undefined' || isEmpty(cardData.longtap_actions))) {
        const serverAction = createServerActionBasedOnTSF({
            binaryTsf: card.LongTapTypedAction?.value,
            productScenario: 'CentaurMainScreen',
            purpose: cardData.type + '_widget_longtap_action',
        });
        cardData.longtap_actions = serverAction ? [{
            log_id: 'main_screen_longtap_server_action',
            url: directivesAction(serverAction),
        }] : undefined;
    }

    // TODO: удалить после перехода на TypedAction (релиза флага из https://st.yandex-team.ru/CENTAUR-1128)
    if (card.Action && typeof cardData.actions === 'undefined') {
        cardData.actions = [{
            log_id: 'main_screen_server_action',
            url: card.Action,
        }];
    }

    // TODO: удалить после перехода на TypedAction (релиза флага из https://st.yandex-team.ru/CENTAUR-1128)
    if (card.LongTapAction && typeof cardData.longtap_actions === 'undefined') {
        cardData.longtap_actions = [{
            log_id: 'main_screen_longtap_server_action',
            url: card.LongTapAction,
        }];
    }

    cardData.colIndex = colIndex;
    cardData.rowIndex = rowIndex;

    return cardData;
}

import { compact } from 'lodash';
import { IYouTubeProps } from './types';
import { EnumLayer } from '../../../../actions/client';
import { ICardDataAdapter } from '../types';

export const getYouTubeCardData: ICardDataAdapter<IYouTubeProps> = function getYouTubeCardData(card, requestState) {
    if (typeof card.YouTubeCardData !== 'undefined' && card.YouTubeCardData !== null) {
        return {
            type: 'youtube',
            actions: compact([
                card.Action && {
                    log_id: 'main_screen_youtube_action',
                    url: card.Action,
                },
            ]),
            layer: EnumLayer.content,
            requestState,
        };
    }
    return null;
};

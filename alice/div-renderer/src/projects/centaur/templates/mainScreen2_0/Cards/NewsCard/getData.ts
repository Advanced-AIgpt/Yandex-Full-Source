import { compact } from 'lodash';
import { INewsCardProps } from './types';
import { EnumLayer } from '../../../../actions/client';
import { getImageByTopic } from '../../../../helpers/newsHelper';
import { ICardDataAdapter } from '../types';

export const getNewsCardData: ICardDataAdapter<INewsCardProps> = function getNewsCardData(card, requestState) {
    if (typeof card.NewsCardData !== 'undefined' && card.NewsCardData !== null) {
        const data = card.NewsCardData;

        return {
            type: 'news',
            title: data.Title,
            content: data.Content,
            image: data.ImageUrl || getImageByTopic(data.Topic) || undefined,
            layer: EnumLayer.dialog,
            requestState,
            actions: compact([
                card.Action && {
                    log_id: 'news_card_action',
                    url: card.Action,
                },
            ]),
        };
    }
    return null;
};

import { sample } from 'lodash';
import { IInfoCardProps } from './types';
import { DISCOVERY_CARDS } from '../../../mainScreen/discovery';
import { ICardDataAdapter } from '../types';
import { textAction } from '../../../../../../common/actions';

export const getInfoCardData: ICardDataAdapter<IInfoCardProps> = function getInfoCardData(card, requestState) {
    if (typeof card.InfoCardData !== 'undefined' && card.InfoCardData !== null) {
        const data = card.InfoCardData.Title === 'test' ? DISCOVERY_CARDS[0] : sample(DISCOVERY_CARDS) || DISCOVERY_CARDS[0];

        return {
            type: 'info',
            title: data.title,
            image_background: data.bg,
            description: data.description,
            requestState,
            actions: [
                {
                    log_id: 'discovery.mainscreen.execute',
                    url: textAction(data.description),
                },
            ],
        };
    }
    return null;
};

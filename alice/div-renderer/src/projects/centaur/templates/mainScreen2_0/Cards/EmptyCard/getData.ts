import { IEmptyCardProps } from './types';
import { ICardDataAdapter } from '../types';

export const getEmptyCardData: ICardDataAdapter<IEmptyCardProps> = function getEmptyCardData(card, requestState) {
    if (card && card.VacantCardData) {
        return {
            type: 'empty',
            requestState,
        };
    }
    return null;
};

import { getCard } from './Cards';
import { getCardMainScreenId } from './Cards/helpers';
import { compact } from 'lodash';
import { NAlice } from '../../../../protos';
type ITCentaurScenarioWidgetData = NAlice.NData.ITCentaurScenarioWidgetData;
import { MMRequest } from '../../../../common/helpers/MMRequest';
import { IRequestState } from '../../../../common/types/common';

export default function MainScreenDivPatch(data: ITCentaurScenarioWidgetData, _: MMRequest, requestState: IRequestState) {
    const res = data.WidgetCards?.map(card => {
        let cardDiv;
        if (card) {
            cardDiv = getCard(card, requestState, {
                colIndex: card?.MainScreenPosition?.Column || 0,
                rowIndex: card?.MainScreenPosition?.Row || 0,
            });
        }
        return {
            id: getCardMainScreenId({
                colIndex: card?.MainScreenPosition?.Column || 0,
                rowIndex: card?.MainScreenPosition?.Row || 0,
            }),
            items: compact([
                cardDiv?.div,
            ]),
        };
    });
    return compact(res);
}

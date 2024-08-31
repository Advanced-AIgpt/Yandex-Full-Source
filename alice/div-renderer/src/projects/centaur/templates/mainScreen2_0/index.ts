import { compact } from 'lodash';
import { Div, TemplateCard, Templates } from 'divcard2';
import MainScreenWrapper from './MainScreenWrapper';
import { getCard } from './Cards';
import { NAlice } from '../../../../protos';
import { IRequestState } from '../../../../common/types/common';
import { MMRequest } from '../../../../common/helpers/MMRequest';

export default function MainScreen2_0(
    { Columns: Columns }: NAlice.NData.ITCentaurMainScreenMyScreenData,
    _: MMRequest,
    requestState: IRequestState,
) {
    if (!Columns) {
        throw new Error('Columns must to be non-empty array');
    }

    const cardsDiv: Div[][] = compact(Columns.map((col, colIndex) => {
        const cards = (
            (Number(col.WidgetCards?.length) > 0 && col.WidgetCards) ||
            (Number(col.Cards?.length) > 0 && col.Cards)
        ) || [];

        return cards?.map((item, rowIndex) => {
            const result = getCard(item, requestState, {
                colIndex,
                rowIndex,
            });

            return result.div;
        });
    }));

    return new TemplateCard(new Templates({}), {
        log_id: 'mainScreen',
        states: [
            {
                state_id: 0,
                div: MainScreenWrapper({ cards: cardsDiv }),
            },
        ],
    });
}

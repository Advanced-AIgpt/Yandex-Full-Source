import { NAlice } from '../../../../../../protos';
import { ISkillCardText } from './types';
type ITSkillResponse = NAlice.NData.TDialogovoSkillCardData.ITSkillResponse;

export function getTextData(card: Readonly<ITSkillResponse>): ISkillCardText | null {
    if (card.TextResponse) {
        return {
            type: 'Text',
            text: card.TextResponse.Text ?? ' ',
        };
    }

    return null;
}

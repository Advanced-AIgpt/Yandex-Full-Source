import { ISkillCardProps } from './types';
import { ICardDataAdapter } from '../types';
import { NAlice } from '../../../../../../protos';
import { EnumLayer } from '../../../../actions/client';
type ITButton = NAlice.NData.TCentaurWidgetCardData.TExternalSkillCardData.TMainScreenData.ITButton;

interface INormalisedButton {
    Text: string;
    Payload?: string;
}

function isNormalButton(el: ITButton): el is INormalisedButton {
    return Boolean(el.Text);
}

export const getSkillCardData: ICardDataAdapter<ISkillCardProps> = function getSkillCardData(card, requestState) {
    if (typeof card.ExternalSkillCardData !== 'undefined' && card.ExternalSkillCardData !== null) {
        const data = card.ExternalSkillCardData;

        if (data.widgetGalleryData) {
            return {
                type: 'skill',
                requestState,
                skillName: data.skillInfo?.Name || ' ',
                skillImage: data.skillInfo?.Logo || ' ',
                skillId: data.skillInfo?.SkillId || ' ',
                title: ' ',
            };
        } else {
            return {
                type: 'skill',
                requestState,
                title: data.mainScreenData?.Title,
                comment: data.mainScreenData?.Text,
                image: data.mainScreenData?.ImageUrl,
                skillName: data.skillInfo?.Name || ' ',
                skillImage: data.skillInfo?.Logo || ' ',
                skillId: data.skillInfo?.SkillId || ' ',
                layer: EnumLayer.dialog,
                buttons: data.mainScreenData?.buttons
                    ?.filter(isNormalButton)
                    .map(el => ({
                        text: el.Text,
                        payload: el.Payload || undefined,
                    })),
            };
        }
    }
    return null;
};

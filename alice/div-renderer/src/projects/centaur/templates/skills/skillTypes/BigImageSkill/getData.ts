import { NAlice } from '../../../../../../protos';
import { ISkillCardButton } from '../../types';
import { ISkillCardBigImage } from './types';
type ITButton = NAlice.NData.TDialogovoSkillCardData.TSkillResponse.ITButton;

function getButtonData(button: ITButton | null | undefined): ISkillCardButton | null {
    if (button) {
        return {
            text: button.Text ?? '',
            url: button.Url ?? '',
            payload: button.Payload ?? null,
        };
    }
    return null;
}

export function getBigImageData(card: NAlice.NData.TDialogovoSkillCardData.ITSkillResponse): ISkillCardBigImage | null {
    if (card.BigImageResponse) {
        return {
            type: 'BigImage',
            image: {
                title: card.BigImageResponse.ImageItem?.Title ?? ' ',
                imageUrl: card.BigImageResponse.ImageItem?.ImageUrl ?? ' ',
                description: card.BigImageResponse.ImageItem?.Description ?? ' ',
                button: getButtonData(card.BigImageResponse.ImageItem?.Button),
            },
        };
    }

    return null;
}

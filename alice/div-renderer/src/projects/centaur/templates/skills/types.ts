import type { ISkillCardText } from './skillTypes/TextSkill/types';
import type { ISkillCardItemsList } from './skillTypes/ItemsListSkill/types';
import type { ISkillCardBigImage } from './skillTypes/BigImageSkill/types';
import { ISkillCardGallery } from './skillTypes/GallerySkill/types';

interface ISkillSuggest {
    text: string,
    url: string,
    payload: null | string,
}

export interface ISkillCardButton {
    text: string,
    url: string,
    payload: string | null,
}

export type ISkillCardType = ISkillCardText | ISkillCardItemsList | ISkillCardBigImage | ISkillCardGallery;

export type CardType = ISkillCardType['type'];

export interface Types<T = ISkillCardType> {
    skillInfo: {
        name: string,
        logo: string,
        dialogId: string,
    },
    request: {
        text: string,
    },
    response: {
        card: T,
        buttons: ISkillCardButton[],
        suggests: ISkillSuggest[],
    },
}

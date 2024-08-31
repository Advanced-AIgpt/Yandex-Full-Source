import { Div, TemplateCard, Templates } from 'divcard2';
import { compact } from 'lodash';
import { suggestButtonHeight } from '../suggests';
import { offsetFromEdgeOfScreen } from '../../style/constants';
import SkillsWrapper from './SkillsWrapper';
import { CardType, ISkillCardType, Types } from './types';
import TextSkill from './skillTypes/TextSkill/TextSkill';
import ItemsListSkill from './skillTypes/ItemsListSkill/ItemsListSkill';
import BigImageSkill from './skillTypes/BigImageSkill/BigImageSkill';
import { NAlice } from '../../../../protos';
import { getTextData } from './skillTypes/TextSkill/getData';
import { getItemListData } from './skillTypes/ItemsListSkill/getData';
import { getBigImageData } from './skillTypes/BigImageSkill/getData';
import GallerySkill from './skillTypes/GallerySkill/GallerySkill';
import { getGalleryData } from './skillTypes/GallerySkill/getData';
type ITDialogovoSkillCardData = NAlice.NData.ITDialogovoSkillCardData;

// в будущем должно приходить в запросе
const clientHeight = 880;
const suggestsBlockHeight = offsetFromEdgeOfScreen * 2 + suggestButtonHeight;

const componentContentMap: {[name in CardType]: (options: {
    data: Types,
    suggestsBlockHeight: number,
    clientHeight: number,
}) => Div} = {
    Text: TextSkill,
    BigImage: BigImageSkill,
    ItemsList: ItemsListSkill,
    Gallery: GallerySkill,
};

function cardAdapter(card: NAlice.NData.TDialogovoSkillCardData.ITSkillResponse): ISkillCardType | null {
    return getTextData(card) ??
        getItemListData(card) ??
        getBigImageData(card) ??
        getGalleryData(card);
}

export function skillsDataAdapter(data: ITDialogovoSkillCardData): Types {
    let cardInfo = data.SkillResponse && cardAdapter(data.SkillResponse);

    if (!cardInfo) {
        cardInfo = {
            type: 'Text',
            text: 'Ошибка. Не удается отобразить содержимое.',
        };
    }

    return {
        skillInfo: {
            name: data.SkillInfo?.Name || ' ',
            logo: data.SkillInfo?.Logo || '',
            dialogId: data.SkillInfo?.SkillId || '',
        },
        response: {
            card: cardInfo,
            buttons: compact(data.SkillResponse?.buttons?.map(
                button => (
                    button.Url && button.Text ?
                        {
                            url: button.Url,
                            text: button.Text,
                            payload: button.Payload || null,
                        } : null
                ),
            )) ||
                [],
            suggests: compact(data.SkillResponse?.suggests?.map(
                suggest => (
                    suggest.Url && suggest.Text ?
                        {
                            url: suggest.Url,
                            text: suggest.Text,
                            payload: suggest.Payload || null,
                        } : null
                ),
            )) ||
                [],
        },
        request: {
            text: data.SkillRequest?.Text || 'Навык',
        },
    };
}

export default function skills(data: ITDialogovoSkillCardData) {
    const params = skillsDataAdapter(data);
    const content = componentContentMap[params.response.card.type]({
        data: params,
        clientHeight,
        suggestsBlockHeight,
    });

    return new TemplateCard(new Templates({}), {
        log_id: 'skill_card',
        states: [
            {
                state_id: 0,
                div: SkillsWrapper({
                    data: params,
                    children: [content],
                }),
            },
        ],
    });
}

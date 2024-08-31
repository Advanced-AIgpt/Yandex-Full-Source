import { FixedSize, GalleryBlock } from 'divcard2';
import { compact } from 'lodash';
import { titleLineHeight } from '../../../../components/TitleLine/TitleLine';
import { Types } from '../../types';
import { ButtonList } from '../../../../components/ButtonList/ButtonList';
import { TextSkillContent } from './components/TextSkillContent';
import { TextSkillTitle } from './components/TextSkillTitle';

interface Props {
    data: Types;
    suggestsBlockHeight: number;
    clientHeight: number;
}

export default function TextSkill({ data, suggestsBlockHeight, clientHeight }: Props) {
    if (data.response.card.type !== 'Text') {
        throw new TypeError(`Invalid input data type "${data.response.card.type}", "Text" is expected`);
    }
    const text = data.response.card.text;
    const requestText = data.request.text;

    return new GalleryBlock({
        paddings: {
            top: titleLineHeight,
            bottom: suggestsBlockHeight,
        },
        height: new FixedSize({ value: clientHeight }),
        orientation: 'vertical',
        items: compact([
            TextSkillTitle(requestText),
            TextSkillContent(text),
            data.response.buttons && ButtonList({ buttons: data.response.buttons }),
        ]),
    });
}

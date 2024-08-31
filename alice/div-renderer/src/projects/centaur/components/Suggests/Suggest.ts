import { ISuggest } from './types';
import { Div, SolidBackground, TextBlock, WrapContentSize } from 'divcard2';
import { title36m } from '../../style/Text/Text';
import { suggestButtonHeight } from '../../templates/suggests';

export default function Suggest({
    text,
    actions,
    colorSet,
}: ISuggest): Div {
    return new TextBlock({
        ...title36m,
        background: [
            new SolidBackground({ color: colorSet.suggestsBackground }),
        ],
        border: { corner_radius: suggestButtonHeight / 2 },
        text,
        text_color: colorSet.textColor,
        paddings: {
            top: 13,
            right: 24,
            bottom: 13,
            left: 24,
        },
        actions,
        width: new WrapContentSize(),
        height: new WrapContentSize(),
    });
}

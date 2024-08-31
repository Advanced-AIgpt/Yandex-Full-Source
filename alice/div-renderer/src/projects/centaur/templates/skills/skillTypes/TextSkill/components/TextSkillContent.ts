import { TextBlock, WrapContentSize } from 'divcard2';
import { offsetFromEdgeOfScreen } from '../../../../../style/constants';
import { closeButtonDefaultSize } from '../../../../../components/CloseButton';
import { textBreakpoint } from '../../../../../helpers/helpers';
import { text40r, title44r, title48m, title56m, title60m, title64m } from '../../../../../style/Text/Text';

const wrapSkillsText = textBreakpoint(
    [90, title64m],
    [130, title60m],
    [175, title56m],
    [240, title48m],
    [280, title44r],
    [Number.POSITIVE_INFINITY, text40r],
);

export function TextSkillContent(text: string) {
    return new TextBlock({
        ...wrapSkillsText(text),
        margins: {
            left: offsetFromEdgeOfScreen,
            right: offsetFromEdgeOfScreen + closeButtonDefaultSize,
        },
        height: new WrapContentSize(),
        alignment_vertical: 'top',
        alignment_horizontal: 'left',
        text,
    });
}

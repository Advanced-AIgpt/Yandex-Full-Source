import { TextBlock } from 'divcard2';
import { upperFirst } from 'lodash';
import { text40r } from '../../../../../style/Text/Text';
import { colorWhiteOpacity50, offsetFromEdgeOfScreen } from '../../../../../style/constants';
import { closeButtonDefaultSize } from '../../../../../components/CloseButton';

export function TextSkillTitle(requestText: string) {
    return new TextBlock({
        ...text40r,
        text_color: colorWhiteOpacity50,
        text: upperFirst(requestText),
        margins: {
            left: offsetFromEdgeOfScreen,
            right: offsetFromEdgeOfScreen + closeButtonDefaultSize,
            bottom: 10,
        },
    });
}

import { Div, FixedSize, MatchParentSize, SeparatorBlock, SolidBackground } from 'divcard2';
import { colorWhiteOpacity15, offsetFromEdgeOfScreen } from '../../../../../style/constants';

export function ItemsListSeparator(): Div {
    return new SeparatorBlock({
        width: new MatchParentSize(),
        height: new FixedSize({ value: 1 }),
        margins: {
            left: offsetFromEdgeOfScreen,
            right: offsetFromEdgeOfScreen,
        },
        background: [new SolidBackground({ color: colorWhiteOpacity15 })],
    });
}

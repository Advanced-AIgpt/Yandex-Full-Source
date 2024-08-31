import { ContainerBlock, MatchParentSize, TextBlock, WrapContentSize } from 'divcard2';
import { ZWSP } from '../../../../Letters';
import { text32r } from '../../style/Text/Text';

export function EmptyServiceTab(): ContainerBlock {
    return new ContainerBlock({
        width: new MatchParentSize(),
        height: new MatchParentSize(),
        content_alignment_horizontal: 'center',
        items: [
            new TextBlock({
                ...text32r,
                text: ZWSP,
                width: new MatchParentSize(),
                height: new WrapContentSize(),
                auto_ellipsize: 1,
                text_alignment_horizontal: 'center',
            }),
        ],
    });
}

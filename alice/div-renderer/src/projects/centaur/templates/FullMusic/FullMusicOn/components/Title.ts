import { TextBlock, WrapContentSize } from 'divcard2';
import { title44m } from '../../../../style/Text/Text';

export function Title(trackId: string, title: string) {
    return new TextBlock({
        ...title44m,
        id: `track_title_info_${trackId}`,
        text: title,
        alignment_horizontal: 'center',
        alignment_vertical: 'center',
        max_lines: 1,
        width: new WrapContentSize(),
        height: new WrapContentSize(),
        margins: {
            right: 24,
            top: 40,
            left: 24,
        },
    });
}

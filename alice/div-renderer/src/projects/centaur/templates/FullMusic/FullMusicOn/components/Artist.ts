import { TextBlock, WrapContentSize } from 'divcard2';
import { text40m } from '../../../../style/Text/Text';
import { colorWhiteOpacity50 } from '../../../../style/constants';

export function Artist(trackId: string, artist: string) {
    return new TextBlock({
        ...text40m,
        id: `track_artist_info_${trackId}`,
        text: artist,
        max_lines: 1,
        text_color: colorWhiteOpacity50,
        alignment_horizontal: 'center',
        alignment_vertical: 'center',
        width: new WrapContentSize(),
        height: new WrapContentSize(),
        margins: {
            right: 24,
            left: 24,
        },
    });
}

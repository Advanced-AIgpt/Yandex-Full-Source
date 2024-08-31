import { TextBlock, WrapContentSize } from 'divcard2';
import { MainMusicInfoProps } from '../../MainMusicInfo';
import { title64m } from '../../../../../../style/Text/Text';

export function TitleBlock({ trackId, title }: Pick<MainMusicInfoProps, 'trackId' | 'title'>) {
    return new TextBlock({
        ...title64m,
        id: `track_title_${trackId}`,
        text: title,
        alignment_vertical: 'top',
        max_lines: 3,
        width: new WrapContentSize(),
        margins: {
            left: 24,
            bottom: 20,
        },
        transition_in: {
            type: 'set',
            items: [
                {
                    type: 'fade',
                    duration: 400,
                    interpolator: 'ease_out',
                },
            ],
        },
        transition_out: {
            type: 'set',
            items: [
                {
                    type: 'fade',
                    duration: 400,
                    interpolator: 'ease_in',
                },
            ],
        },
    });
}

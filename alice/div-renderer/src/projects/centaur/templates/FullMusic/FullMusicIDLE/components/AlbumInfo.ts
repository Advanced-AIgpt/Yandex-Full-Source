import { ContainerBlock, MatchParentSize, TextBlock, WrapContentSize } from 'divcard2';
import { title44m, title60m } from '../../../../style/Text/Text';

interface Props {
    trackId: string;
    title: string;
    artist: string;
}

export function AlbumInfo({
    trackId,
    title,
    artist,
}: Props) {
    return new ContainerBlock({
        orientation: 'vertical',
        width: new MatchParentSize({ weight: 100 }),
        height: new WrapContentSize(),
        alignment_vertical: 'center',
        margins: {
            left: 24,
        },
        items: [
            new TextBlock({
                ...title60m,
                id: `track_title_${trackId}`,
                text: title,
                alignment_vertical: 'top',
                max_lines: 2,
                width: new WrapContentSize(),
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
            }),
            new TextBlock({
                ...title44m,
                id: `track_artist_${trackId}`,
                text: artist,
                max_lines: 1,
                text_color: '#77ffffff',
                width: new MatchParentSize(),
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
            }),
        ],
    });
}

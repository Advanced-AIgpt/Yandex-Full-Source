import { MatchParentSize, TextBlock } from 'divcard2';
import { MainMusicInfoProps } from '../../MainMusicInfo';
import { text40m } from '../../../../../../style/Text/Text';

export function ArtistBlock({
    trackId,
    artist,
}: Pick<MainMusicInfoProps, 'trackId' | 'artist'>) {
    return new TextBlock({
        ...text40m,
        id: `track_artist_${trackId}`,
        text: artist,
        max_lines: 1,
        text_color: '#77ffffff',
        width: new MatchParentSize(),
        margins: {
            left: 24,
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

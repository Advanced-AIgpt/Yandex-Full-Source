import { ContainerBlock, FixedSize, ImageBlock, MatchParentSize, SolidBackground, TextBlock } from 'divcard2';
import { offsetFromEdgeOfScreen } from '../../../../style/constants';
import { MusicImages } from '../../images';
import { text40m } from '../../../../style/Text/Text';
import { AudioPlayerDivConstants } from '../../constants';
import { setStateAction } from '../../../../../../common/actions/div';

interface HeaderProps {
    audio_source_id: string;
    header?: string;
}

export function Header({
    header,
}: HeaderProps) {
    return new ContainerBlock({
        orientation: 'horizontal',
        width: new MatchParentSize(),
        height: new FixedSize({ value: 184 }),
        content_alignment_horizontal: 'right',
        paddings: {
            top: offsetFromEdgeOfScreen,
            bottom: offsetFromEdgeOfScreen,
            left: offsetFromEdgeOfScreen,
            right: offsetFromEdgeOfScreen,
        },
        items: [
            new TextBlock({
                ...text40m,
                text: header || ' ',
                text_color: '#88898A',
                max_lines: 1,
                alignment_vertical: 'center',
                width: new MatchParentSize({ weight: 1 }),
                margins: {
                    right: 24,
                },
            }),
            new ContainerBlock({
                orientation: 'overlap',
                action: {
                    log_id: 'audio_player_next_screen',
                    url: setStateAction(`0/${AudioPlayerDivConstants.MUSIC_SCREEN_ID}/${AudioPlayerDivConstants.MUSIC_SCREEN_ON_ID}`),
                },
                width: new FixedSize({ value: 88 }),
                height: new FixedSize({ value: 88 }),
                background: [
                    new SolidBackground({ color: '#1AFFFFFF' }),
                ],
                border: {
                    corner_radius: 20,
                },
                margins: {
                    right: 112,
                },
                items: [
                    new ImageBlock({
                        image_url: MusicImages.audioPlayerDots,
                        width: new FixedSize({ value: 56 }),
                        height: new FixedSize({ value: 56 }),
                        alignment_vertical: 'center',
                        alignment_horizontal: 'center',
                    }),
                ],
            }),
        ],
    });
}

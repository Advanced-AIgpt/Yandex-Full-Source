import { ContainerBlock, Div, DivStateBlock, FixedSize, ImageBlock, MatchParentSize } from 'divcard2';
import { AudioPlayerDivConstants } from '../../../../constants';
import { MusicImages } from '../../../../images';

export function BigLikeOnCover(): Div {
    return new ContainerBlock({
        orientation: 'overlap',
        width: new MatchParentSize(),
        height: new MatchParentSize(),
        items: [
            new DivStateBlock({
                div_id: AudioPlayerDivConstants.LIKED_SWITCHER_COVER_LIKE_ID,
                width: new MatchParentSize(),
                height: new MatchParentSize(),
                states: [
                    {
                        state_id: AudioPlayerDivConstants.LIKED_SWITCHER_COVER_LIKE_OFF_ID,
                        div: new ImageBlock({
                            alpha: 0.0,
                            image_url: MusicImages.audioPlayerBigLike,
                            width: new FixedSize({ value: 232 }),
                            height: new FixedSize({ value: 232 }),
                            alignment_vertical: 'center',
                            alignment_horizontal: 'center',
                        }),
                        animation_out: {
                            name: 'set',
                            items: [
                                {
                                    name: 'fade',
                                    duration: 400,
                                    start_value: 0.0,
                                    end_value: 1.0,
                                    interpolator: 'ease_in',
                                },
                                {
                                    name: 'scale',
                                    duration: 400,
                                    start_value: 1.0,
                                    end_value: 1.6,
                                    interpolator: 'ease_in',
                                },
                            ],
                        },
                    },
                    {
                        state_id: AudioPlayerDivConstants.LIKED_SWITCHER_COVER_LIKE_ON_ID,
                        div: new ImageBlock({
                            alpha: 0.0,
                            image_url: MusicImages.audioPlayerBigLike,
                            width: new FixedSize({ value: 371 }),
                            height: new FixedSize({ value: 371 }),
                            alignment_vertical: 'center',
                            alignment_horizontal: 'center',
                        }),
                        animation_in: {
                            name: 'fade',
                            duration: 400,
                            start_value: 1.0,
                            end_value: 0.0,
                            start_delay: 400,
                            interpolator: 'ease_out',
                        },
                    },
                ],
            }),
        ],
    });
}

import { ContainerBlock, DivStateBlock, FixedSize, ImageBlock, MatchParentSize } from 'divcard2';
import { setStateAction } from '../../../../../../common/actions/div';
import { AudioPlayerDivConstants } from '../../constants';
import { MusicImages } from '../../images';

interface Props {
    trackId: string;
    isLiked: boolean;
}

export function PlayerLike({
    trackId,
    isLiked,
}: Props) {
    const stateLikeId = `${AudioPlayerDivConstants.LIKED_SWITCHER_ID}-${trackId}`;
    return new ContainerBlock({
        orientation: 'overlap',
        id: `track_like_${trackId}`,
        width: new FixedSize({ value: 98 }),
        height: new FixedSize({ value: 98 }),
        margins: {
            left: 2,
        },
        alignment_vertical: 'center',
        items: [
            new DivStateBlock({
                div_id: stateLikeId,
                width: new MatchParentSize(),
                height: new MatchParentSize(),
                margins: {
                    left: 24,
                },
                default_state_id: isLiked ?
                    AudioPlayerDivConstants.LIKED_SWITCHER_ON_ID :
                    AudioPlayerDivConstants.LIKED_SWITCHER_OFF_ID,
                states: [
                    {
                        state_id: AudioPlayerDivConstants.LIKED_SWITCHER_OFF_ID,
                        div: new ImageBlock({
                            image_url: MusicImages.unliked,
                            width: new FixedSize({ value: 56 }),
                            height: new FixedSize({ value: 56 }),
                            alignment_vertical: 'center',
                            alignment_horizontal: 'center',
                            actions: [
                                {
                                    log_id: 'audio_player_set_like',
                                    url: setStateAction([
                                        '0',
                                        AudioPlayerDivConstants.MUSIC_SCREEN_ID,
                                        AudioPlayerDivConstants.MUSIC_SCREEN_OFF_ID,
                                        stateLikeId,
                                        AudioPlayerDivConstants.LIKED_SWITCHER_ON_ID,
                                    ]),
                                },
                                {
                                    log_id: 'audio_player_set_like',
                                    url: setStateAction([
                                        '0',
                                        AudioPlayerDivConstants.MUSIC_SCREEN_ID,
                                        AudioPlayerDivConstants.MUSIC_SCREEN_OFF_ID,
                                        AudioPlayerDivConstants.LIKED_SWITCHER_COVER_LIKE_ID,
                                        AudioPlayerDivConstants.LIKED_SWITCHER_COVER_LIKE_ON_ID,
                                    ]),
                                },
                            ],
                        }),
                        animation_in: {
                            name: 'scale',
                            start_value: 1.2,
                            end_value: 1.0,
                            duration: 200,
                            start_delay: 200,
                            interpolator: 'ease_out',
                        },
                        animation_out: {
                            name: 'scale',
                            start_value: 1.0,
                            end_value: 1.4,
                            duration: 200,
                            interpolator: 'ease_in',
                        },
                    },
                    {
                        state_id: AudioPlayerDivConstants.LIKED_SWITCHER_ON_ID,
                        div: new ImageBlock({
                            image_url: MusicImages.liked,
                            width: new FixedSize({ value: 56 }),
                            height: new FixedSize({ value: 56 }),
                            alignment_vertical: 'center',
                            alignment_horizontal: 'center',
                            actions: [
                                {
                                    log_id: 'audio_player_set_like',
                                    url: setStateAction([
                                        '0',
                                        AudioPlayerDivConstants.MUSIC_SCREEN_ID,
                                        AudioPlayerDivConstants.MUSIC_SCREEN_OFF_ID,
                                        stateLikeId,
                                        AudioPlayerDivConstants.LIKED_SWITCHER_OFF_ID,
                                    ]),
                                },
                                {
                                    log_id: 'audio_player_set_like',
                                    url: setStateAction([
                                        '0',
                                        AudioPlayerDivConstants.MUSIC_SCREEN_ID,
                                        AudioPlayerDivConstants.MUSIC_SCREEN_OFF_ID,
                                        AudioPlayerDivConstants.LIKED_SWITCHER_COVER_LIKE_ID,
                                        AudioPlayerDivConstants.LIKED_SWITCHER_COVER_LIKE_OFF_ID,
                                    ]),
                                },
                            ],
                        }),
                        animation_in: {
                            name: 'scale',
                            start_value: 1.4,
                            end_value: 1.0,
                            duration: 200,
                            start_delay: 200,
                            interpolator: 'ease_out',
                        },
                        animation_out: {
                            name: 'scale',
                            start_value: 1.0,
                            end_value: 1.2,
                            duration: 200,
                            interpolator: 'ease_in',
                        },
                    },
                ],
            }),
        ],
    });
}

import { ContainerBlock, DivStateBlock, FixedSize, ImageBlock, MatchParentSize } from 'divcard2';
import { AudioPlayerDivConstants } from '../../constants';
import { MusicImages } from '../../images';
import { ActionMusicNext } from '../../actions';
import { setStateAction } from '../../../../../../common/actions/div';

interface Props {
    isDisliked: boolean;
    trackId: string;
}

export function PlayerDislike({
    isDisliked,
    trackId,
}: Props) {
    const stateDislikeId = `${AudioPlayerDivConstants.DISLIKED_SWITCHER_ID}-${trackId}`;
    return new ContainerBlock({
        orientation: 'overlap',
        width: new FixedSize({ value: 88 }),
        height: new FixedSize({ value: 88 }),
        border: {
            corner_radius: 20,
        },
        margins: {
            right: 18,
        },
        alignment_vertical: 'center',
        items: [
            new DivStateBlock({
                div_id: stateDislikeId,
                default_state_id: isDisliked ?
                    AudioPlayerDivConstants.DISLIKED_SWITCHER_ON_ID :
                    AudioPlayerDivConstants.DISLIKED_SWITCHER_OFF_ID,
                states: [
                    {
                        state_id: AudioPlayerDivConstants.DISLIKED_SWITCHER_OFF_ID,
                        div: new ImageBlock({
                            actions: [
                                {
                                    log_id: 'audio_player_dislike_on',
                                    url: setStateAction([
                                        '0',
                                        AudioPlayerDivConstants.MUSIC_SCREEN_ID,
                                        AudioPlayerDivConstants.MUSIC_SCREEN_OFF_ID,
                                        stateDislikeId,
                                        AudioPlayerDivConstants.DISLIKED_SWITCHER_ON_ID,
                                    ]),
                                },
                                {
                                    log_id: 'audio_player_dislike_on_next_track',
                                    url: ActionMusicNext(),
                                },
                            ],
                            image_url: MusicImages.dislike,
                            width: new FixedSize({ value: 56 }),
                            height: new FixedSize({ value: 56 }),
                            alignment_vertical: 'center',
                            alignment_horizontal: 'center',
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
                            end_value: 1.4,
                            duration: 200,
                            interpolator: 'ease_in',
                        },
                    },
                    {
                        state_id: AudioPlayerDivConstants.DISLIKED_SWITCHER_ON_ID,
                        div: new ImageBlock({
                            actions: [
                                {
                                    log_id: 'audio_player_dislike_off',
                                    url: setStateAction([
                                        '0',
                                        AudioPlayerDivConstants.MUSIC_SCREEN_ID,
                                        AudioPlayerDivConstants.MUSIC_SCREEN_OFF_ID,
                                        stateDislikeId,
                                        AudioPlayerDivConstants.DISLIKED_SWITCHER_OFF_ID,
                                    ]),
                                },
                            ],
                            image_url: MusicImages.dislikeActive,
                            width: new FixedSize({ value: 56 }),
                            height: new FixedSize({ value: 56 }),
                            alignment_vertical: 'center',
                            alignment_horizontal: 'center',
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
                            end_value: 1.4,
                            duration: 200,
                            interpolator: 'ease_in',
                        },
                    },
                ],
                width: new MatchParentSize(),
                height: new MatchParentSize(),
            }),
        ],
    });
}

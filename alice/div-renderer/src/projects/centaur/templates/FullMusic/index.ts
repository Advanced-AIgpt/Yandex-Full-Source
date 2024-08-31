import { DivStateBlock, IDivData, MatchParentSize } from 'divcard2';
import FullMusicOff from './FullMusicOff/FullMusicOff';
import FullMusicOn from './FullMusicOn/FullMusicOn';
import FullMusicIDLE from './FullMusicIDLE/FullMusicIDLE';
import { AudioPlayerDivConstants } from './constants';
import { TopLevelCard } from '../../helpers/helpers';
import { AdapterGetMusicDataFromTrack, ITMusicPlayerData } from '../Music/dataAdapter';
import { setStateAction } from '../../../../common/actions/div';

export function FullMusic(musicPlayerData: ITMusicPlayerData) {
    const {
        coverUri,
        trackId,
        artist,
        audio_source_id,
        header,
        title,
        isShuffle,
        repeatMode,
        isLiked,
        isDisliked,
    } = AdapterGetMusicDataFromTrack(musicPlayerData);

    const div: IDivData = {
        log_id: 'audio_player_wrapper',
        transition_animation_selector: 'data_change',
        states: [
            {
                state_id: 0,
                div: new DivStateBlock({
                    height: new MatchParentSize(),
                    div_id: AudioPlayerDivConstants.MUSIC_SCREEN_ID,
                    transition_animation_selector: 'data_change',
                    states: [
                        FullMusicOff({
                            direction: 'bottom',
                            title,
                            artist,
                            audio_source_id,
                            trackId,
                            coverUri,
                            header,
                            isLiked,
                            isDisliked,
                        }),
                        FullMusicOn({
                            artist,
                            title,
                            trackId,
                            coverUri,
                            audio_source_id,
                            isShuffle,
                            repeatMode,
                        }),
                        FullMusicIDLE({
                            coverUri,
                            trackId,
                        }),
                    ],
                }),
            },
        ],
        variable_triggers: [
            {
                // @ts-ignore
                conditions: '@{isAudioPlaying}',
                conditions_lua: 'return appState:isAudioPlaying()',
                actions: [
                    {
                        log_id: 'trigger_action_play_on_idle_screen',
                        url: setStateAction([
                            '0',
                            AudioPlayerDivConstants.MUSIC_SCREEN_ID,
                            AudioPlayerDivConstants.MUSIC_SCREEN_IDLE_ID,
                            AudioPlayerDivConstants.PLAY_PAUSE_SWITCHER_ID,
                            AudioPlayerDivConstants.PLAY_PAUSE_SWITCHER_PAUSE_ID,
                        ]),
                    },
                    {
                        log_id: 'trigger_action_play_on_off_screen',
                        url: setStateAction([
                            '0',
                            AudioPlayerDivConstants.MUSIC_SCREEN_ID,
                            AudioPlayerDivConstants.MUSIC_SCREEN_OFF_ID,
                            AudioPlayerDivConstants.PLAY_PAUSE_SWITCHER_ID,
                            AudioPlayerDivConstants.PLAY_PAUSE_SWITCHER_PAUSE_ID,
                        ]),
                    },
                ],
            },
            {
                // @ts-ignore
                conditions: '!@{isAudioPlaying}',
                conditions_lua: 'return not appState:isAudioPlaying()',
                actions: [
                    {
                        log_id: 'trigger_action_pause',
                        url: setStateAction([
                            '0',
                            AudioPlayerDivConstants.MUSIC_SCREEN_ID,
                            AudioPlayerDivConstants.MUSIC_SCREEN_IDLE_ID,
                            AudioPlayerDivConstants.PLAY_PAUSE_SWITCHER_ID,
                            AudioPlayerDivConstants.PLAY_PAUSE_SWITCHER_PLAY_ID,
                        ]),
                    },
                    {
                        log_id: 'trigger_action_pause2',
                        url: setStateAction([
                            '0',
                            AudioPlayerDivConstants.MUSIC_SCREEN_ID,
                            AudioPlayerDivConstants.MUSIC_SCREEN_OFF_ID,
                            AudioPlayerDivConstants.PLAY_PAUSE_SWITCHER_ID,
                            AudioPlayerDivConstants.PLAY_PAUSE_SWITCHER_PLAY_ID,
                        ]),
                    },
                ],
            },
            {
                // @ts-ignore
                conditions: '@{inactivityTimeout} >= 10',
                conditions_lua: 'return appState:inactivityTimeout() >= 10',
                actions: [
                    {
                        log_id: 'trigger_action_lava',
                        url: setStateAction([
                            '0',
                            AudioPlayerDivConstants.MUSIC_SCREEN_ID,
                            AudioPlayerDivConstants.MUSIC_SCREEN_IDLE_ID,
                        ]),
                    },
                ],
            },
        ],
    };

    return TopLevelCard(div);
}

import { localCommands } from '../../actions/client';

export const ActionMusicScreenNext = (audioSourceId: string) => localCommands([
    {
        type: 'music_screen_next',
        audio_source_id: audioSourceId,
    },
]);

export const ActionMusicDislike = (audioSourceId: string) => localCommands([
    {
        type: 'audio_player_proxy_dislike',
        audio_source_id: audioSourceId,
    },
    'io_service_dislike',
    'io_service_next',
]);

export const ActionMusicLike = (audioSourceId: string) => localCommands([
    {
        type: 'audio_player_proxy_like',
        audio_source_id: audioSourceId,
    },
]);

export const ActionMusicExitFromIDLEMode = () => localCommands([
    'audio_player_proxy_exit_from_idle_mode',
]);

export const ActionMusicPlayPayse = () => localCommands([
    'io_service_toggle_play_pause',
]);

export const ActionMusicNext = () => localCommands([
    'audio_player_proxy_next',
    'io_service_next',
]);

export const ActionMusicPrev = () => localCommands([
    'audio_player_proxy_prev',
    'io_service_prev',
]);

export const ActionMusicShuffle = () => localCommands([
    'audio_player_proxy_shuffle',
]);

export const ActionMusicRepeat = () => localCommands([
    'audio_player_proxy_repeat',
]);

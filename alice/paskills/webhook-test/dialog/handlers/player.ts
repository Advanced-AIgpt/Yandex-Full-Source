import { IncomingMessage } from '../transport';
import { wrapResponse } from '../utils';
import { songsByToken } from '../fixtures';

export const handlePlayerEvent = (incomingMessage: IncomingMessage) => {
    if (incomingMessage.request.type === 'AudioPlayer.PlaybackStarted') {
    }

    if (incomingMessage.request.type === 'AudioPlayer.PlaybackStopped') {
    }

    if (incomingMessage.request.type === 'AudioPlayer.PlaybackNearlyFinished') {
        let token: keyof typeof songsByToken = 'token';
        let song = songsByToken[token];
        let offset_ms = 5000;
        let text = 'Предзагруженная песня с отступом 5 секунд';
        let end_session = false;
        let with_text = true;

        if (incomingMessage.state?.audio_player?.token === 'token1') {
            token = 'token2';
            song = songsByToken[token];
            offset_ms = 3000;
            text = 'Предзагруженная песня с отступом 3 секунды';
            end_session = false;
        }

        if (incomingMessage.state?.audio_player?.token === 'token2') {
            token = 'token1';
            song = songsByToken[token];
            offset_ms = 0;
            text = 'Предзагруженная песня без отступа';
            end_session = false;
        }

        if (incomingMessage.state?.audio_player?.token === 'token3') {
            token = 'token4';
            song = songsByToken[token];
            offset_ms = 0;
            with_text = false;
            end_session = true;
        }

        if (incomingMessage.state?.audio_player?.token === 'token4') {
            token = 'token3';
            song = songsByToken[token];
            offset_ms = 0;
            with_text = false;
            end_session = true;
        }

        return wrapResponse(
            incomingMessage,
            {
                text: with_text ? text : undefined,
                tts: with_text ? text : undefined,
                should_listen: false,
                directives: {
                    audio_player: {
                        action: 'Play',
                        item: {
                            stream: {
                                url: song.url,
                                offset_ms: offset_ms,
                                token: token,
                            },
                            metadata: song.metadata,
                        },
                    },
                },
                end_session: end_session,
            },
            incomingMessage.state?.session,
            {
                ...incomingMessage.state?.user,
                audioPlayer: incomingMessage.state?.audio_player,
            },
        );
    }

    if (incomingMessage.request.type === 'AudioPlayer.PlaybackFinished') {
    }

    if (incomingMessage.request.type === 'AudioPlayer.PlaybackFailed') {
    }

    return wrapResponse(
        incomingMessage,
        {
            text: '',
            should_listen: false,
            end_session: false,
        },
        incomingMessage.state?.session,
        {
            ...incomingMessage.state?.user,
            audioPlayer: incomingMessage.state?.audio_player,
        },
    );
};

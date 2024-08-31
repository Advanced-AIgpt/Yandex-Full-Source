package ru.yandex.alice.paskill.dialogovo.external.v1.request.audio;

import lombok.EqualsAndHashCode;
import lombok.Getter;

import ru.yandex.alice.paskill.dialogovo.external.v1.request.InputType;

@Getter
@EqualsAndHashCode(callSuper = true)
public class AudioPlayerFailedRequest extends AudioPlayerEventRequest {

    private final AudioPlayerError error;

    public AudioPlayerFailedRequest(AudioPlayerError error) {
        super(InputType.AUDIO_PLAYER_PLAYBACK_FAILED);
        this.error = error;
    }
}

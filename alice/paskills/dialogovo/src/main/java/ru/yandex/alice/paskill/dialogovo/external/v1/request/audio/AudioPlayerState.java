package ru.yandex.alice.paskill.dialogovo.external.v1.request.audio;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;

import ru.yandex.alice.kronstadt.core.domain.AudioPlayerActivityState;

@Data
public class AudioPlayerState {

    private final String token;

    @JsonProperty(value = "offset_ms")
    private final long offsetMs;

    @JsonProperty(value = "state")
    private final AudioPlayerActivityState audioPlayerActivityState;
}

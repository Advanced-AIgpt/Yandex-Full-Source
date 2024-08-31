package ru.yandex.alice.paskill.dialogovo.external.v1.response.audio;

import com.fasterxml.jackson.annotation.JsonValue;
import lombok.Getter;

public enum AudioPlayerActionType {
    PLAY("Play"),
    STOP("Stop"),
    REWIND("Rewind"),
    CLEAR_QUEUE("ClearQueue");

    @Getter
    @JsonValue
    private final String type;

    AudioPlayerActionType(String type) {
        this.type = type;
    }
}

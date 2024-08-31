package ru.yandex.alice.paskill.dialogovo.external.v1.request.audio;

import com.fasterxml.jackson.annotation.JsonInclude;
import lombok.Data;

import static com.fasterxml.jackson.annotation.JsonInclude.Include.NON_ABSENT;

@Data
@JsonInclude(NON_ABSENT)
public class AudioPlayerError {
    public static final AudioPlayerError DEFAULT = new AudioPlayerError("error",
            AudioPlayerErrorType.MEDIA_ERROR_UNKNOWN);

    private final String message;
    private final AudioPlayerErrorType type;
}

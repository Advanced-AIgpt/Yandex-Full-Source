package ru.yandex.alice.paskill.dialogovo.external.v1.response.audio;

import java.util.Optional;

import javax.validation.Valid;
import javax.validation.constraints.NotNull;

import com.fasterxml.jackson.annotation.JsonInclude;
import lombok.Data;

import static com.fasterxml.jackson.annotation.JsonInclude.Include.NON_ABSENT;

@Data
@JsonInclude(NON_ABSENT)
public class AudioItem {
    @NotNull
    @Valid
    private final AudioStream stream;

    private final Optional<@Valid AudioMetadata> metadata;

    public AudioStream getStream() {
        return stream;
    }

    public Optional<AudioMetadata> getMetadata() {
        return metadata;
    }
}

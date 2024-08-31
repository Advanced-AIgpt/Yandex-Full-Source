package ru.yandex.alice.paskill.dialogovo.external.v1.response.audio;

import java.util.Optional;

import javax.validation.Valid;
import javax.validation.constraints.NotNull;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;
import lombok.EqualsAndHashCode;

@Data
@EqualsAndHashCode(callSuper = true)
public class Play extends AudioPlayerAction {

    @NotNull
    @JsonProperty(value = "item")
    @Valid
    private final AudioItem audioItem;

    @JsonProperty(value = "background_mode")
    @Valid
    private final Optional<BackgroundMode> backgroundMode;

    public AudioItem getAudioItem() {
        return audioItem;
    }

    public Optional<BackgroundMode> getBackgroundMode() {
        return backgroundMode;
    }

    @Override
    public AudioPlayerActionType getAction() {
        return AudioPlayerActionType.PLAY;
    }
}

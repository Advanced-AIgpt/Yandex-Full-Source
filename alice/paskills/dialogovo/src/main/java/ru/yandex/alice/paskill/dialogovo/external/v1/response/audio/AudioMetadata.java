package ru.yandex.alice.paskill.dialogovo.external.v1.response.audio;

import java.util.Optional;

import javax.validation.Valid;
import javax.validation.constraints.Size;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;

import ru.yandex.alice.paskill.dialogovo.external.v1.response.Image;

@Data
public class AudioMetadata {

    private final Optional<@Size(max = 1024) String> title;

    @JsonProperty("sub_title")
    private final Optional<@Size(max = 1024) String> subTitle;

    @JsonProperty(value = "art")
    private final Optional<@Valid Image> art;

    @JsonProperty(value = "background_image")
    private final Optional<@Valid Image> backgroundImage;

    public Optional<String> getTitle() {
        return title;
    }

    public Optional<String> getSubTitle() {
        return subTitle;
    }

    public Optional<Image> getArt() {
        return art;
    }

    public Optional<Image> getBackgroundImage() {
        return backgroundImage;
    }
}

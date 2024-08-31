package ru.yandex.alice.paskill.dialogovo.external.v1.response;

import java.util.Optional;

import javax.annotation.Nonnull;
import javax.validation.Valid;
import javax.validation.constraints.NotNull;
import javax.validation.constraints.Size;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.AllArgsConstructor;
import lombok.Data;
import lombok.EqualsAndHashCode;

import ru.yandex.alice.paskill.dialogovo.domain.Censored;
import ru.yandex.alice.paskill.dialogovo.utils.MdsImageId;

@Data
@AllArgsConstructor
@EqualsAndHashCode(callSuper = true)
public class BigImageCard extends Card {

    private final Optional<@Valid CardButton> button;
    @NotNull
    @MdsImageId
    @JsonProperty(value = "image_id")
    // not final to be processed by annotation walker
    private String imageId;
    @Censored
    private final Optional<@Size(max = 128) String> title;
    @Censored
    private final Optional<@Size(max = 1024) String> description;

    // Не является частью внешнего API
    // используется в некоторых внутренних навыках (например, "Угадай город по фото")
    // для переопределения неймспейса аватарницы, в котором лежат картинки
    @JsonProperty("mds_namespace")
    private final Optional<@Size(max = 256) String> mdsNamespace;

    public Optional<CardButton> getButton() {
        return button;
    }

    @Nonnull
    public String getImageId() {
        return imageId;
    }

    public Optional<String> getTitle() {
        return title;
    }

    public Optional<String> getDescription() {
        return description;
    }

    public Optional<String> getMdsNamespace() {
        return mdsNamespace;
    }
}

package ru.yandex.alice.paskill.dialogovo.external.v1.response;

import java.util.Optional;

import javax.validation.Valid;
import javax.validation.constraints.Size;

import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.AllArgsConstructor;
import lombok.Data;

import ru.yandex.alice.paskill.dialogovo.domain.Censored;
import ru.yandex.alice.paskill.dialogovo.utils.MdsImageId;

import static com.fasterxml.jackson.annotation.JsonInclude.Include.NON_ABSENT;

@Data
@Censored
@AllArgsConstructor
@JsonInclude(NON_ABSENT)
public class ItemsListItem {

    private final Optional<@Valid CardButton> button;
    @JsonProperty(value = "image_id")
    private Optional<@MdsImageId String> imageId;
    @Censored
    private Optional<@Size(max = 128) String> title;
    @Censored
    private Optional<@Size(max = 256) String> description;

    // Не является частью внешнего API
    // используется в некоторых внутренних навыках (например, "Угадай город по фото")
    // для переопределения неймспейса аватарницы, в котором лежат картинки
    @JsonProperty("mds_namespace")
    private final Optional<@Size(max = 256) String> mdsNamespace;

    public Optional<CardButton> getButton() {
        return button;
    }

    public Optional<String> getImageId() {
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

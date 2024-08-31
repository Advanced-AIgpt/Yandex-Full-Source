package ru.yandex.alice.paskill.dialogovo.external.v1.response;

import java.util.List;
import java.util.Optional;

import javax.annotation.Nullable;
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
public class BigImageListCard extends Card {

    @Nullable
    @Valid
    private final ItemsListCardHeader header;

    @Valid
    @NotNull
    @Size(min = 1, max = 10)
    private final List<Item> items;

    public Optional<ItemsListCardHeader> getHeader() {
        return Optional.ofNullable(header);
    }

    public List<Item> getItems() {
        return items;
    }

    @Data
    @Censored
    @AllArgsConstructor
    public static class Item {
        @JsonProperty(value = "image_id")
        @NotNull
        @MdsImageId
        private String imageId;
        @Censored
        private Optional<@Size(max = 128) String> title;
        @Censored
        private Optional<@Size(max = 256) String> description;
        private final Optional<@Valid CardButton> button;

        // Не является частью внешнего API
        // используется в некоторых внутренних навыках (например, "Угадай город по фото")
        // для переопределения неймспейса аватарницы, в котором лежат картинки
        @JsonProperty("mds_namespace")
        private final Optional<@Size(max = 256) String> mdsNamespace;

        public String getImageId() {
            return imageId;
        }

        public Optional<String> getTitle() {
            return title;
        }

        public Optional<String> getDescription() {
            return description;
        }

        public Optional<CardButton> getButton() {
            return button;
        }

        public Optional<String> getMdsNamespace() {
            return mdsNamespace;
        }
    }
}

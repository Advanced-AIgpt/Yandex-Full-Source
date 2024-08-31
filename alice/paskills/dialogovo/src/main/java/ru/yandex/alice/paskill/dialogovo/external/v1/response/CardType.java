package ru.yandex.alice.paskill.dialogovo.external.v1.response;

import com.fasterxml.jackson.annotation.JsonValue;
import lombok.Getter;

public enum CardType {
    BIG_IMAGE("BigImage"),
    ITEMS_LIST("ItemsList"),
    @Deprecated
    BIG_IMAGE_LIST("BigImageList"), // deprecated. use image gallery
    IMAGE_GALLERY("ImageGallery"),
    INVALID_CARD("#INVALID_CARD#");

    @Getter
    @JsonValue
    private final String type;

    CardType(String type) {
        this.type = type;
    }
}

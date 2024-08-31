package ru.yandex.alice.kronstadt.core.layout.div.block

import com.fasterxml.jackson.annotation.JsonValue

enum class BlockType(@JsonValue val type: String) {
    IMAGE("div-image-block"),
    BUTTONS("div-buttons-block"),
    TRAFFIC("div-traffic-block"),
    FOOTER("div-footer-block"),
    TABLE("div-table-block"),
    SEPARATOR("div-separator-block"),
    UNIVERSAL("div-universal-block"),
    TITLE("div-title-block"),
    CONTAINER("div-container-block"),
    TABS("div-tabs-block"),
    GALLERY("div-gallery-block");
}

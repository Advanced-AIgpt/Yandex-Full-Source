package ru.yandex.alice.kronstadt.core.layout.div

import com.fasterxml.jackson.annotation.JsonValue

enum class BackgroundType(@field:JsonValue val type: String) {
    SOLID("div-solid-background"),
    GRADIENT("div-gradient-background"),
    IMAGE("div-image-background");
}

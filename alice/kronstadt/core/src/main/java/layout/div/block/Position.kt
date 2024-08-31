package ru.yandex.alice.kronstadt.core.layout.div.block

import com.fasterxml.jackson.annotation.JsonValue

enum class Position(@JsonValue val pos: String) {
    LEFT("left"),
    RIGHT("right"),
    CENTER("center"),
    AUTO("auto");
}

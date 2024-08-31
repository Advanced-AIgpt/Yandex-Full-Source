package ru.yandex.alice.kronstadt.core.layout.div

import com.fasterxml.jackson.annotation.JsonValue

enum class DivElementType(@field:JsonValue val type: String) {
    IMAGE("div-image-element"),
    BUTTON("div-button-element"),
    SEPARATOR("separator_element"),
    ROW("row_element"),
    DATE("date_element");
}

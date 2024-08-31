package ru.yandex.alice.kronstadt.core.layout.div

import com.fasterxml.jackson.annotation.JsonValue

enum class Size(@JsonValue val size: String) {
    ZERO("zero"),
    XXS("xxs"),
    XS("xs"),
    S("s"),
    M("m"),
    L("l"),
    XL("xl"),
    XXL("xxl"),
    MATCH_PARENT("match_parent");
}

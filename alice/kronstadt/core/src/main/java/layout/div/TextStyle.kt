package ru.yandex.alice.kronstadt.core.layout.div

import com.fasterxml.jackson.annotation.JsonValue

enum class TextStyle(@JsonValue val style: String) {
    TITLE_S("title_s"),
    TITLE_M("title_m"),
    TITLE_L("title_l"),
    TEXT_S("text_s"),
    TEXT_M("text_m"),
    TEXT_M_MEDIUM("text_m_medium"),
    TEXT_L("text_l"),
    NUMBER_S("numbers_s"),
    NUMBER_M("numbers_m"),
    NUMBER_L("numbers_l"),
    BUTTON("button"),
    CARD_HEADER("card_header");
}

package ru.yandex.alice.kronstadt.core.text

import java.util.Locale

enum class Language(val code: String, val locale: Locale) {
    RUSSIAN("ru", Locale.forLanguageTag("ru")),
    UNKNOWN("unknown", Locale.getDefault());

    companion object {
        @JvmStatic
        fun all(): List<Language> = values().filter { language: Language -> language != UNKNOWN }
    }
}

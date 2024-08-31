package ru.yandex.alice.kronstadt.core.text.nlg

import java.util.regex.Pattern

object TemplateToTextConverter {
    private val ACCENT = Pattern.compile("\\+(\\w+)", Pattern.UNICODE_CHARACTER_CLASS)

    @JvmStatic
    fun stripMarkup(tts: String): String = ACCENT.matcher(tts).replaceAll("$1")
}

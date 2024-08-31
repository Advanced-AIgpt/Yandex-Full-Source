package ru.yandex.alice.kronstadt.core.text.nlg

import java.util.regex.Pattern

object TemplateToTtsConverter {
    private val YANDEX_SERVICE_NAME = Pattern.compile(
        "(яндекс)\\.(\\w+)",
        Pattern.UNICODE_CASE or Pattern.UNICODE_CHARACTER_CLASS or Pattern.CASE_INSENSITIVE
    )

    @JvmStatic
    fun ttsify(template: String): String {
        return YANDEX_SERVICE_NAME.matcher(template).replaceAll("$1 $2")
    }
}

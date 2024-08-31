package ru.yandex.alice.kronstadt.core.layout.div

import org.springframework.web.util.HtmlUtils

class Color private constructor() {
    companion object {
        const val WHITE = "#FFFFFF"
        const val GRAY_DEFAULT = "#818181"
        const val BLUE_LINK = "#0A4DC3"
        const val BLUE_BUTTON = "#0078d7"
        const val BLUE_MOBILE = "#6839cf"

        @JvmStatic
        fun coloredText(str: String, color: String): String {
            return "<font color=\"" + color + "\">" + HtmlUtils.htmlEscape(str) + "</font>"
        }
    }

    init {
        throw UnsupportedOperationException()
    }
}

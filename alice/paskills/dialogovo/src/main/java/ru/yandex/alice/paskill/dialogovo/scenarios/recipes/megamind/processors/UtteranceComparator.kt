package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.processors

import java.util.Locale

internal class UtteranceComparator private constructor() {
    init {
        throw UnsupportedOperationException()
    }

    companion object {
        private val regex = "[^а-яА-Я0-9a-zA-Z]+".toRegex()

        private fun normalize(s: String): String = s.lowercase(Locale.ROOT).replace(regex, "").trim()

        @JvmStatic
        fun equal(s1: String?, s2: String?): Boolean {
            return if (s1 == null || s2 == null) {
                s1 == null && s2 == null
            } else {
                normalize(s1) == normalize(s2)
            }
        }
    }
}

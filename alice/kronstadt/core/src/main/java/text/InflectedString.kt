package ru.yandex.alice.kronstadt.core.text

import com.fasterxml.jackson.annotation.JsonCreator
import com.fasterxml.jackson.annotation.JsonValue
import ru.yandex.alice.kronstadt.core.text.InflectedString.RuCase
import ru.yandex.alice.kronstadt.core.utils.StringEnum
import ru.yandex.alice.kronstadt.core.utils.StringEnumResolver

class InflectedString @JsonCreator constructor(private val casedStrings: Map<RuCase, String>) {

    operator fun get(ruCase: RuCase): String {
        return casedStrings[ruCase] ?: (casedStrings[DEFAULT_RU_CASE] ?: "")
    }

    enum class RuCase(@field:JsonValue private val value: String) : StringEnum {
        //именительный
        NOMINATIVE("nom"),

        //родительный
        GENITIVE("gen"),

        //дательный
        DATIVE("dat"),

        //винительный
        ACCUSATIVE("acc"),

        //творительный
        CREATIVE("cre"),

        //предложный
        PREPOSITIONAL("prep");

        override fun value(): String {
            return value
        }

        companion object {
            @JvmField
            val R = StringEnumResolver.resolver(RuCase::class.java)
        }
    }

    companion object {
        private val DEFAULT_RU_CASE = RuCase.NOMINATIVE

        @JvmStatic
        fun cons(nominative: String): InflectedString {
            return InflectedString(mapOf(RuCase.NOMINATIVE to nominative))
        }

        @JvmStatic
        fun cons(casedStrings: Map<RuCase, String>): InflectedString {
            return InflectedString(casedStrings)
        }
    }
}

package ru.yandex.alice.kronstadt.core.layout.div.block

import com.fasterxml.jackson.annotation.JsonProperty
import com.fasterxml.jackson.annotation.JsonValue

data class PredefinedDivSize
private constructor(@JsonProperty val value: PredefinedValue) : DivSize("predefined") {

    enum class PredefinedValue(@JsonValue val code: String) {
        MATCH_PARENT("match_parent"),
        WRAP_CONTENT("wrap_content");
    }

    companion object {
        @JvmField
        val MATCH_PARENT = PredefinedDivSize(PredefinedValue.MATCH_PARENT)

        @JvmField
        val WRAP_CONTENT = PredefinedDivSize(PredefinedValue.WRAP_CONTENT)
    }
}

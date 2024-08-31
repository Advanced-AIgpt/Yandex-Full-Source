package ru.yandex.alice.kronstadt.core.layout.div.block

import com.fasterxml.jackson.annotation.JsonProperty

class NumericDivSize private constructor(
    @JsonProperty val value: Int,
    @JsonProperty val unit: SizeUnit
) : DivSize("numeric") {
    enum class SizeUnit {
        dp, sp
    }

    companion object {
        @JvmStatic
        fun dp(value: Int): NumericDivSize {
            return NumericDivSize(value, SizeUnit.dp)
        }

        fun sp(value: Int): NumericDivSize {
            return NumericDivSize(value, SizeUnit.sp)
        }
    }
}

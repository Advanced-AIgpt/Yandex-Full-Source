package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.nlg

import ru.yandex.alice.kronstadt.core.layout.TextWithTts
import java.math.BigDecimal
import java.math.RoundingMode

class CountableObject(
    private val one: TextWithTts,
    private val few: TextWithTts,
    private val many: TextWithTts,
    override val inflectedName: TextWithTts,
    private val ttsTag: TtsTag
) : Countable, WithDecimalQuantity {

    override fun getPluralForm(number: BigDecimal): TextWithTts {
        val plural: TextWithTts
        val floor = number.setScale(0, RoundingMode.FLOOR).toInt()
        val remainder: Int = floor % 10
        plural = if (floor == 0 && number.remainder(BigDecimal.ONE) != BigDecimal.ZERO) {
            inflectedName
        } else if (floor == 1) {
            one
        } else if ((floor in 2..4) || (floor > 20 && remainder in 1..4)) {
            few
        } else {
            many
        }
        return plural
    }

    override fun pluralize(number: BigDecimal): TextWithTts {
        val plural = getPluralForm(number)
        val numberString = decimalToString(number)
        val ttsTagString = if (numberString.shouldUseTtsTag) ttsTag.tag() else ""
        return TextWithTts(
            String.format("%s %s", numberString.value.text, plural.text).trim(),
            String.format("%s%s %s", ttsTagString, numberString.value.tts, plural.tts).trim()
        )
    }

    private fun decimalToString(decimal: BigDecimal): DecimalString {
        val integer = decimal.setScale(0, RoundingMode.FLOOR)
        val fractional = decimal.remainder(BigDecimal.ONE)
        return if (fractional in CUSTOM_TEXTS) {
            val customText = CUSTOM_TEXTS[fractional]!!
            if (BigDecimal.ZERO == integer) {
                DecimalString(
                    TextWithTts(decimal.toString(), customText.nominative),
                    false
                )
            } else {
                DecimalString(
                    TextWithTts(
                        decimal.toString(), String.format("%d с %s", integer.toInt(), customText.ablative)
                    ),
                    true
                )
            }
        } else if (BigDecimal.ZERO.compareTo(fractional) == 0) {
            DecimalString(
                TextWithTts(
                    integer.toInt().toString(),
                    integer.toInt().toString()
                ),
                true
            )
        } else {
            DecimalString(TextWithTts(decimal.toString()), true)
        }
    }

    private data class DecimalString(val value: TextWithTts, val shouldUseTtsTag: Boolean)
    private data class InflectedCustomText(
        // именительный падеж
        val nominative: String,
        // творительный падеж: с кем/чем
        val ablative: String
    )

    companion object {
        private val CUSTOM_TEXTS: Map<BigDecimal, InflectedCustomText> = mapOf(
            BigDecimal("0.5") to InflectedCustomText("половина", "половиной"),
            BigDecimal("0.33") to InflectedCustomText("одна треть", "одной третью"),
            BigDecimal("0.25") to InflectedCustomText("четверть", "четвертью")
        )

        fun fromTextWithTtsList(
            forms: List<TextWithTts>,
            ttsTag: TtsTag,
            inflectedName: TextWithTts
        ): CountableObject {
            return if (forms.size == 3) {
                CountableObject(
                    forms[0],
                    forms[1],
                    forms[2],
                    inflectedName,
                    ttsTag
                )
            } else {
                throw IllegalArgumentException("forms should contain exactly 3 elements")
            }
        }
    }
}

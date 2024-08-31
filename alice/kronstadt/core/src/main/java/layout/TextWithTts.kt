package ru.yandex.alice.kronstadt.core.layout

import com.fasterxml.jackson.annotation.JsonProperty
import org.springframework.util.StringUtils
import ru.yandex.alice.kronstadt.core.domain.Voice
import java.util.Locale

class TextWithTts @JvmOverloads constructor(
    val text: String,
    @JsonProperty("tts") tts: String? = text
) {

    val tts: String = tts ?: text

    fun getTts(voice: Voice): String {
        return voice.say(tts)
    }

    @JvmOverloads
    fun plus(other: TextWithTts, delimiter: String = " "): TextWithTts {
        return TextWithTts(
            join(text, other.text, delimiter),
            join(tts, other.tts, delimiter)
        )
    }

    fun toLowerCase(): TextWithTts {
        return TextWithTts(
            text = text.lowercase(Locale.ROOT),
            tts = tts.lowercase(Locale.ROOT),
        )
    }

    fun uncapitalize(): TextWithTts {
        return TextWithTts(
            StringUtils.uncapitalize(text),
            StringUtils.uncapitalize(tts)
        )
    }

    fun capitalize(): TextWithTts {
        return TextWithTts(
            StringUtils.capitalize(text),
            StringUtils.capitalize(tts)
        )
    }

    override fun equals(other: Any?): Boolean {
        if (this === other) return true
        if (javaClass != other?.javaClass) return false

        other as TextWithTts

        if (text != other.text) return false
        if (tts != other.tts) return false

        return true
    }

    override fun hashCode(): Int {
        var result = text.hashCode()
        result = 31 * result + tts.hashCode()
        return result
    }

    override fun toString(): String {
        return "TextWithTts(text='$text', tts='$tts')"
    }

    companion object {
        @JvmField
        val EMPTY = TextWithTts("", "")
    }
}

private fun join(a: String, b: String, delimiter: String): String {
    return when {
        a.isEmpty() -> b
        b.isEmpty() -> a
        else -> a + delimiter + b
    }
}

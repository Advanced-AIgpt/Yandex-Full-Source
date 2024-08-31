package ru.yandex.alice.kronstadt.core.domain

import com.fasterxml.jackson.annotation.JsonIgnore
import com.fasterxml.jackson.annotation.JsonValue

enum class Voice(@field:JsonValue val code: String, @field:JsonIgnore val attrs: Map<String, String>? = null) {
    SHITOVA_US("shitova.us"),
    OKSANA("good_oksana"),
    OKSANA_GPU("oksana.gpu"),
    JANE("jane"),
    JANE_GPU("jane.gpu"),
    ZAHAR("zahar"),
    ZAHAR_GPU("zahar.gpu"),
    ERMIL("ermil"),
    ERKANYAVAS("erkanyavas"),
    KOLYA_GPU("kolya.gpu"),
    KOSTYA_GPU("kostya.gpu"),

    // Володя
    VALTZ_GPU("valtz.gpu"),
    ERMIL_GPU("ermil.gpu"),

    //ВТБ Бренд войс Облака
    VTB_BRAND_VOICE("vtb_brand_voice.cloud", mapOf("speed" to "1.2")),

    // Аня
    TATYANA_ABRAMOVA_GPU("tatyana_abramova.gpu");

    @field:JsonIgnore
    private val voiceTag: String

    init {
        val attrString: String =
            this.attrs?.entries?.joinToString(separator = " ", prefix = " ") { (key, value) -> "$key=\"$value\"" } ?: ""
        this.voiceTag = "<speaker voice=\"${this.code}\"${attrString.trimEnd()}>"
    }

    fun say(tts: String): String {
        return voiceTag + tts
    }

    companion object {
        private val codeMap: Map<String, Voice> = Voice.values().associateBy { it.code }

        fun byCode(code: String): Voice? = codeMap[code]
    }
}

// TODO: rename to say() after all .say() calls are moved to kotlin
fun Voice?.sayOrDefault(tts: String?): String? {
    if (this == null || tts == null) {
        return tts;
    }
    return this.say(tts);
}

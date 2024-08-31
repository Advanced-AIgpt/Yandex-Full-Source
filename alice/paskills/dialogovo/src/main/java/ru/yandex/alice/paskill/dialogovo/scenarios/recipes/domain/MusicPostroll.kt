package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain

import ru.yandex.alice.kronstadt.core.layout.TextWithTts
import java.util.Optional

data class MusicPostroll(val text: Optional<TextWithTts>) {

    companion object {
        fun fromJson(
            text: String?,
            tts: String?
        ): MusicPostroll = if (text == null) {
            MusicPostroll(Optional.empty<TextWithTts>())
        } else {
            MusicPostroll(Optional.of(TextWithTts(text, tts)))
        }
    }
}

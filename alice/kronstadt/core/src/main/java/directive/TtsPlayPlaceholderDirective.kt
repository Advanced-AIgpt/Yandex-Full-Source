package ru.yandex.alice.kronstadt.core.directive

/**
 * Used to order tts-answer among other directives
 * By default - answer output is before other directives(audio_stop f.e.) on device
 * But sometimes we want directive finishes before tts from answer
 * Case: We don't want audio_stop to be preventable - so play tts after audio_stop
 * So with this directive audio_stop we can change order - with directives:[audio_stop, tts_play_placeholder]
 */
data class TtsPlayPlaceholderDirective(
    val channel: DirectiveChannel? = null
) : MegaMindDirective {

    enum class DirectiveChannel {
        DIALOG,
        CONTENT,
    }

    companion object {
        @JvmField
        val TTS_PLAY_PLACEHOLDER_DIRECTIVE = TtsPlayPlaceholderDirective()

        @JvmField
        val TTS_PLAY_PLACEHOLDER_DIRECTIVE_DIALOG = TtsPlayPlaceholderDirective(DirectiveChannel.DIALOG)
    }
}

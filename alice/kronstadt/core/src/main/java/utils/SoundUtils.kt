package ru.yandex.alice.kronstadt.core.utils

object SoundUtils {
    private const val SPEAKER_AUDIO_OPUS_FORMAT = "<speaker audio='dialogs-upload/%s/%s.opus'>"

    @JvmStatic
    fun opusTts(skillId: String, soundId: String): String {
        return String.format(SPEAKER_AUDIO_OPUS_FORMAT, skillId, soundId)
    }
}

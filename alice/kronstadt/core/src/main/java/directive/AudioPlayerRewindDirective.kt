package ru.yandex.alice.kronstadt.core.directive

class AudioPlayerRewindDirective(val rewind: Rewind) : MegaMindDirective {
    data class Rewind(
        val type: RewindType,
        val amountMs: Long,
    )

    enum class RewindType {
        FORWARD,
        BACKWARD,
        ABSOLUTE,
    }
}

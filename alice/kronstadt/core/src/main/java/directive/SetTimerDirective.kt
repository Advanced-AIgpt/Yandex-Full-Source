package ru.yandex.alice.kronstadt.core.directive

import java.time.Duration

data class SetTimerDirective @JvmOverloads constructor(
    val duration: Duration,
    val isListeningIsPossible: Boolean = true
) : MegaMindDirective

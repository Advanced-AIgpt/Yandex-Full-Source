package ru.yandex.alice.kronstadt.core.directive

data class MordoviaShowDirective(
    val url: String,
    val isFullScreen: Boolean,
    val viewKey: String
) : MegaMindDirective

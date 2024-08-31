package ru.yandex.alice.kronstadt.core.directive

data class MordoviaCommandDirective(
    val command: String,
    val meta: Map<String, *>,
    val viewKey: String
) : MegaMindDirective

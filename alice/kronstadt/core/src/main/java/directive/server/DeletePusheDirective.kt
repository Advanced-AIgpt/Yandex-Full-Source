package ru.yandex.alice.kronstadt.core.directive.server

data class DeletePusheDirective(
    val pushTag: String
) : ServerDirective

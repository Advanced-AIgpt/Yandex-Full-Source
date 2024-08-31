package ru.yandex.alice.kronstadt.core.input

interface UtteranceInput {
    val originalUtterance: String
    val normalizedUtterance: String
}

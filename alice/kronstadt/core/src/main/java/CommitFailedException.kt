package ru.yandex.alice.kronstadt.core

class CommitFailedException : RuntimeException {
    constructor(message: String, cause: Exception) : super(message, cause)
    constructor(message: String) : super(message)
}

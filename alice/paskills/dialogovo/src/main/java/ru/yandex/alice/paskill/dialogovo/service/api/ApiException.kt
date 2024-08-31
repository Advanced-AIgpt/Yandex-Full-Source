package ru.yandex.alice.paskill.dialogovo.service.api

class ApiException : RuntimeException {
    internal constructor()
    internal constructor(message: String) : super(message)
    internal constructor(message: String, cause: Throwable) : super(message, cause)
    internal constructor(cause: Throwable) : super(cause)
    internal constructor(
        message: String,
        cause: Throwable,
        enableSuppression: Boolean,
        writableStackTrace: Boolean
    ) : super(message, cause, enableSuppression, writableStackTrace)
}

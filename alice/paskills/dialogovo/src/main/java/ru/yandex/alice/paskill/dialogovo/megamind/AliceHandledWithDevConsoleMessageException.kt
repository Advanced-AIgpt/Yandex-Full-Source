package ru.yandex.alice.paskill.dialogovo.megamind

import ru.yandex.alice.kronstadt.core.AliceHandledException
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfoAction

open class AliceHandledWithDevConsoleMessageException @JvmOverloads constructor(
    message: String,
    // debugging message for dev-console
    val externalDebugMessage: String,
    responseBody: ExceptionResponseBody,
    cause: Exception? = null,
) : AliceHandledException(message, responseBody, cause) {

    constructor(
        message: String,
        action: AnalyticsInfoAction,
        debugMessage: String,
        cause: Exception
    ) : this(
        message = message,
        cause = cause,
        externalDebugMessage = debugMessage,
        responseBody = ExceptionResponseBody(action = action)
    )

    @JvmOverloads
    constructor(
        message: String,
        action: AnalyticsInfoAction,
        debugMessage: String,
        aliceText: String = DEFAULT_TEXT,
        aliceSpeech: String = aliceText,
        expectRequest: Boolean = false,
        cause: Exception? = null,
    ) : this(
        message = message,
        externalDebugMessage = debugMessage,
        responseBody = ExceptionResponseBody(action, aliceText, aliceSpeech, expectRequest),
        cause = cause
    )
}

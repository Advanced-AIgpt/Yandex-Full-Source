package ru.yandex.alice.kronstadt.core

import com.fasterxml.jackson.annotation.JsonProperty
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfoAction

open class AliceHandledException @JvmOverloads constructor(
    message: String,
    val responseBody: ExceptionResponseBody,
    cause: Exception? = null,
) : RuntimeException(message, cause) {

    /*constructor(
        message: String,
        action: AnalyticsInfoAction,
        debugMessage: String,
        cause: Exception
    ) :
        this(
            message = message,
            cause = cause,
            responseBody = ExceptionResponseBody(action = action)
        )*/

    @JvmOverloads
    constructor(
        message: String,
        action: AnalyticsInfoAction,
        aliceText: String = DEFAULT_TEXT,
        aliceSpeech: String = aliceText,
        expectRequest: Boolean = false,
        cause: Exception? = null,
    ) : this(
        message = message,
        responseBody = ExceptionResponseBody(action, aliceText, aliceSpeech, expectRequest),
        cause = cause
    )

    companion object {
        const val DEFAULT_TEXT = "Произошла ошибка"

        @JvmStatic
        fun from(
            message: String,
            action: AnalyticsInfoAction,
            aliceText: String,
            expectRequest: Boolean
        ): AliceHandledException {
            return AliceHandledException(
                message = message,
                responseBody = ExceptionResponseBody(
                    action = action,
                    aliceText = aliceText,
                    aliceSpeech = aliceText,
                    expectRequest = expectRequest
                ),
                cause = null
            )
        }
    }

    data class ExceptionResponseBody(
        @JsonProperty("analytics_info_action")
        val action: AnalyticsInfoAction,
        @JsonProperty("alice_text")
        val aliceText: String = DEFAULT_TEXT,
        @JsonProperty("alice_speech")
        val aliceSpeech: String = aliceText,
        @JsonProperty("expect_request")
        val expectRequest: Boolean = false,
    )
}

package ru.yandex.alice.paskill.dialogovo.webhook.client

import ru.yandex.alice.paskill.dialogovo.megamind.AliceHandledWithDevConsoleMessageException
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.ErrorAnalyticsInfoAction

class WebhookException(
    val request: WebhookRequestParams,
    val response: WebhookRequestResult,
    cause: Exception? = null
) : AliceHandledWithDevConsoleMessageException(
    message = "Webhook error: " + response.formatErrors(),
    action = ErrorAnalyticsInfoAction.REQUEST_FAILURE,
    debugMessage = "Не удалось получить корректный ответ от навыка",
    aliceText = "Извините, навык не отвечает",
    aliceSpeech = "Извините, навык не отвечает",
    expectRequest = true,
    cause = cause
)

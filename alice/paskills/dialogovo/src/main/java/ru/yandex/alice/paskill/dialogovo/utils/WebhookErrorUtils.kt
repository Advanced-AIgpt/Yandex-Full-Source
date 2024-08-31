package ru.yandex.alice.paskill.dialogovo.utils

import ru.yandex.alice.paskill.dialogovo.domain.Session
import ru.yandex.alice.paskill.dialogovo.external.WebhookErrorCode
import ru.yandex.alice.paskill.dialogovo.webhook.client.WebhookRequestResult
import java.util.Optional

object WebhookErrorUtils {

    @JvmStatic val WEBHOOK_ERRORS_LIMIT = 1

    @JvmStatic
    fun exceededErrorsLimit(result: WebhookRequestResult, session: Optional<Session>): Boolean {
        return !(
            result.errors.size == 1 &&
                result.errors[0].code().equals(WebhookErrorCode.TIME_OUT) &&
                (session.isEmpty || session.get().failCounter < WEBHOOK_ERRORS_LIMIT)
            )
    }
}

package ru.yandex.alice.paskill.dialogovo.service.abuse

import ru.yandex.alice.paskill.dialogovo.external.v1.response.WebhookResponse

interface AbuseApplier {
    fun apply(response: WebhookResponse)
}

package ru.yandex.alice.paskill.dialogovo.external.v1.request

import ru.yandex.alice.paskill.dialogovo.service.appmetrica.MetricaEvent
import ru.yandex.alice.paskill.dialogovo.service.appmetrica.MetricaEventConstants

class TeasersRequest : RequestBase(InputType.TEASERS)

class TeasersWebhookRequest(
    meta: Meta,
    session: WebhookSession,
    request: TeasersRequest,
    state: WebhookState?
): WebhookRequest<TeasersRequest>(meta, session, request, state) {
    override fun getMetricaEvent(): MetricaEvent {
        return MetricaEventConstants.teasersMetricaEvent
    }
}

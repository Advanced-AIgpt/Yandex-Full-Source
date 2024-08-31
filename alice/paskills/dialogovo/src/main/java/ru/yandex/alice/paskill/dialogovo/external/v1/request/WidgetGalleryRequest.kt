package ru.yandex.alice.paskill.dialogovo.external.v1.request

import ru.yandex.alice.paskill.dialogovo.service.appmetrica.MetricaEvent
import ru.yandex.alice.paskill.dialogovo.service.appmetrica.MetricaEventConstants

class WidgetGalleryRequest: RequestBase(InputType.WIDGET_GALLERY)

class WidgetGalleryWebhookRequest(
    meta: Meta,
    session: WebhookSession,
    request: WidgetGalleryRequest,
    state: WebhookState?
): WebhookRequest<WidgetGalleryRequest>(meta, session, request, state) {
    override fun getMetricaEvent(): MetricaEvent {
        return MetricaEventConstants.widgetGalleryMetricaEvent
    }
}

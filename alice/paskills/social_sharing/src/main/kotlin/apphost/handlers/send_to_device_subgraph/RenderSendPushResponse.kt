package ru.yandex.alice.social.sharing.apphost.handlers.send_to_device_subgraph

import NMatrix.NApi.Delivery
import org.springframework.stereotype.Component
import ru.yandex.alice.social.sharing.ApphostHandler
import ru.yandex.alice.social.sharing.apphost.handlers.NOTIFICATOR_HTTP_RESPONSE
import ru.yandex.alice.social.sharing.apphost.handlers.SEND_PUSH_RESPONSE
import ru.yandex.alice.social.sharing.proto.context.SendToDevice
import ru.yandex.monlib.metrics.labels.Labels
import ru.yandex.monlib.metrics.registry.MetricRegistry
import ru.yandex.web.apphost.api.request.RequestContext

@Component
class RenderSendPushResponse(
    private val metricRegistry: MetricRegistry,
): ApphostHandler {

    override val path = "/send_to_device_subgraph/render_response"

    override fun handle(context: RequestContext) {
        val notificatorResponse = parseHttpResponse(
            context,
            NOTIFICATOR_HTTP_RESPONSE
        ) { data -> Delivery.TDeliveryResponse.parseFrom(data) }
        metricRegistry.counter(
            "notificator_response",
            Labels.of("code", notificatorResponse.code.name)
        ).inc()
        val response = SendToDevice.TSendPushResponse.newBuilder()
            .setNotificatorResponse(notificatorResponse)
            .build()
        context.addProtobufItem(SEND_PUSH_RESPONSE, response)
    }
}

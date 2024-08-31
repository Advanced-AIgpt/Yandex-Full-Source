package ru.yandex.alice.social.sharing.apphost.handlers.send_to_device

import NAppHostHttp.Http
import com.fasterxml.jackson.databind.ObjectMapper
import com.google.protobuf.ByteString
import org.springframework.stereotype.Component
import ru.yandex.alice.social.sharing.ApphostHandler
import ru.yandex.alice.social.sharing.apphost.handlers.DEFAULT_HEADERS_JSON
import ru.yandex.alice.social.sharing.apphost.handlers.PROTO_HTTP_RESPONSE
import ru.yandex.alice.social.sharing.apphost.handlers.SEND_PUSH_RESPONSE
import ru.yandex.alice.social.sharing.proto.context.SendToDevice
import ru.yandex.web.apphost.api.request.RequestContext

@Component
class RenderSendToDeviceHttpResponse(
    private val objectMapper: ObjectMapper
): ApphostHandler {

    override val path = "/send_to_device/render_http_response"

    override fun handle(context: RequestContext) {
        val sendPushResponse = context
            .getSingleRequestItem(SEND_PUSH_RESPONSE)
            .getProtobufData(SendToDevice.TSendPushResponse.getDefaultInstance())
        val status = SendToDeviceStatus.fromNotificatorResponseCode(sendPushResponse.notificatorResponse.code)
        val responseBody = SendToDeviceResponseBody(status)
        val httpResponse = Http.THttpResponse.newBuilder()
            .setStatusCode(if (status == SendToDeviceStatus.OK) 200 else 500)
            .addAllHeaders(DEFAULT_HEADERS_JSON)
            .setContent(ByteString.copyFrom(objectMapper.writeValueAsBytes(responseBody)))
            .build()
        context.addProtobufItem(PROTO_HTTP_RESPONSE, httpResponse)
    }
}

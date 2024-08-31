package ru.yandex.alice.social.sharing.apphost.handlers.list_devices

import NAppHostHttp.Http
import com.fasterxml.jackson.databind.ObjectMapper
import com.google.protobuf.ByteString
import org.springframework.stereotype.Component
import ru.yandex.alice.social.sharing.ApphostHandler
import ru.yandex.alice.social.sharing.apphost.handlers.DEFAULT_HEADERS_JSON
import ru.yandex.alice.social.sharing.apphost.handlers.LIST_DEVICES_RESPONSE
import ru.yandex.alice.social.sharing.apphost.handlers.PROTO_HTTP_RESPONSE
import ru.yandex.alice.social.sharing.proto.context.ListDevicesProto
import ru.yandex.web.apphost.api.request.RequestContext

@Component
class ListDevicesRenderHttpResponse(
    private val objectMapper: ObjectMapper,
) : ApphostHandler {
    override val path = "/list_devices/render_http_response"

    override fun handle(context: RequestContext) {
        val response = context
            .getSingleRequestItem(LIST_DEVICES_RESPONSE)
            .getProtobufData(ListDevicesProto.TListDevicesResponse.getDefaultInstance())
        val responseBody = objectMapper.writeValueAsBytes(
            ListDevicesResponse(
                response.devicesList.map {
                    ListDevicesResponse.Device(
                        it.deviceId,
                        it.room,
                        it.type,
                        it.iconUrl,
                        it.name,
                    )
                }
            )
        )
        context.addProtobufItem(
            PROTO_HTTP_RESPONSE,
            Http.THttpResponse.newBuilder()
                .setStatusCode(200)
                .addAllHeaders(DEFAULT_HEADERS_JSON)
                .setContent(ByteString.copyFrom(responseBody))
                .build()
        )
    }
}

package ru.yandex.alice.social.sharing.apphost.handlers.list_devices

import NAlice.NNotificator.Api
import NAppHostHttp.Http
import com.fasterxml.jackson.databind.ObjectMapper
import org.apache.logging.log4j.LogManager
import org.springframework.http.HttpHeaders
import org.springframework.stereotype.Component
import ru.yandex.alice.social.sharing.ApphostHandler
import ru.yandex.alice.social.sharing.apphost.handlers.BLACKBOX_RESPONSE
import ru.yandex.alice.social.sharing.apphost.handlers.LIST_DEVICES_REQUEST
import ru.yandex.alice.social.sharing.apphost.handlers.blackbox.BlackboxResponse
import ru.yandex.alice.social.sharing.jsonBodyParser
import ru.yandex.alice.social.sharing.proto.context.ListDevicesProto
import ru.yandex.web.apphost.api.request.RequestContext

@Component
class ListDevicesPrepareRequests(
        private val objectMapper: ObjectMapper,
): ApphostHandler {
    override val path = "/list_devices/prepare_requests"

    override fun handle(context: RequestContext) {
        val blackboxResponseO = context.getSingleRequestItemO(BLACKBOX_RESPONSE)
        if (!blackboxResponseO.isPresent) {
            logger.info("Won't send IoT request: no $BLACKBOX_RESPONSE")
            return;
        }
        val blackboxResponse = parseHttpResponse(
            context,
                BLACKBOX_RESPONSE,
            objectMapper.jsonBodyParser(BlackboxResponse::class.java)
        )

        val puid = blackboxResponse.defaultUid
        if (puid == null) {
            logger.info("Won't send IoT request: no default uid in blackbox response")
            return;
        }

        val listDevicesRequest = context.getSingleRequestItemO(LIST_DEVICES_REQUEST)
            .map { item -> item.getProtobufData(ListDevicesProto.TListDevicesRequest.getDefaultInstance()) }
            .orElse(null)

        putNotificatorRequest(context, puid, listDevicesRequest)

        val userTicket = blackboxResponse.userTicket
        if (userTicket == null) {
            logger.info("Won't send IoT request: no user ticket in blackbox response")
            return
        }
        putIotRequest(context, userTicket)
    }

    private fun putNotificatorRequest(
        context: RequestContext,
        puid: String,
        listDevicesRequest: ListDevicesProto.TListDevicesRequest?
    ) {
        val bodyBuilder = Api.TGetDevicesRequest.newBuilder()
            .setPuid(puid)

        if (!listDevicesRequest?.requiredFeaturesList.isNullOrEmpty()) {
            bodyBuilder.addAllSupportedFeatures(listDevicesRequest?.requiredFeaturesList)
        }

        val request = Http.THttpRequest.newBuilder()
            .setMethod(Http.THttpRequest.EMethod.Post)
            .setPath("/devices")
            .addHeaders(
                Http.THeader.newBuilder()
                    .setName(HttpHeaders.CONTENT_TYPE)
                    .setValue("application/x-protobuf")
            )
            .setContent(bodyBuilder.build().toByteString())
        context.addProtobufItem("notificator_device_list_http_request", request.build())
    }

    private fun putIotRequest(context: RequestContext, userTicket: String) {
        val iotRequest: Http.THttpRequest = Http.THttpRequest.newBuilder()
            .setPath("/v1.0/user/info")
            .addHeaders(Http.THeader.newBuilder()
                .setName("X-Ya-User-Ticket")
                .setValue(userTicket)
            )
            .build()
        context.addProtobufItem("iot_device_list_http_request", iotRequest)
    }

    companion object {
        private val logger = LogManager.getLogger()
    }

}

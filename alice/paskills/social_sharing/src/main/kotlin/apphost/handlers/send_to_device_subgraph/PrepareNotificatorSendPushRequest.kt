package ru.yandex.alice.social.sharing.apphost.handlers.send_to_device_subgraph

import NMatrix.NApi.Delivery
import NAppHostHttp.Http
import com.fasterxml.jackson.databind.ObjectMapper
import com.google.protobuf.util.JsonFormat
import org.apache.logging.log4j.LogManager
import org.springframework.beans.factory.annotation.Value
import org.springframework.stereotype.Component
import ru.yandex.alice.social.sharing.ApphostHandler
import ru.yandex.alice.social.sharing.apphost.handlers.NOTIFICATOR_HTTP_REQUEST
import ru.yandex.alice.social.sharing.apphost.handlers.SEND_PUSH_REQUEST
import ru.yandex.alice.social.sharing.proto.context.SendToDevice
import ru.yandex.web.apphost.api.request.RequestContext

@Component
class PrepareNotificatorSendPushRequest(
    protected val objectMapper: ObjectMapper,
    @Value("\${notificator.push.ttl}") protected val pushMessageTtl: Int,
): ApphostHandler {

    override val path = "/send_to_device_subgraph/prepare_notificator_request"

    override fun handle(context: RequestContext) {
        val sendPushRequest = context
            .getSingleRequestItem(SEND_PUSH_REQUEST)
            .getProtobufData(SendToDevice.TSendPushRequest.getDefaultInstance())
        logger.info("Creating notificator request")

        val deviceId = sendPushRequest.deviceId
        if (deviceId == null) {
            logger.error("Won't send push request to device: device id is null")
            return
        }

        val directive = sendPushRequest.directive
        if (directive == null) {
            logger.error("Won't send push request to device: directive is null")
            return
        }

        val puid = sendPushRequest.uid
        if (puid == null) {
            logger.error("Won't send push request to device: uid is null")
            return
        }

        val notificatorRequestBody = Delivery.TDelivery.newBuilder()
            .setPuid(puid)
            .setDeviceId(deviceId)
            .setTtl(pushMessageTtl)
            .setSemanticFrameRequestData(directive)
            .build()
        logger.info("Notificator request body:\n{}", JsonFormat.printer().print(notificatorRequestBody))
        val notificatorHttpRequest = Http.THttpRequest.newBuilder()
            .setMethod(Http.THttpRequest.EMethod.Post)
            .setPath("/delivery/push")
            .setContent(notificatorRequestBody.toByteString())
            .build()
        context.addProtobufItem(NOTIFICATOR_HTTP_REQUEST, notificatorHttpRequest)
    }

    companion object {
        private val logger = LogManager.getLogger()
    }

}

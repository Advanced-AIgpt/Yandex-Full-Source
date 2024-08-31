package ru.yandex.alice.social.sharing.apphost.handlers

import NAppHostHttp.Http
import com.fasterxml.jackson.databind.ObjectMapper
import org.apache.logging.log4j.LogManager
import ru.yandex.alice.megamind.protos.common.Atm
import ru.yandex.alice.megamind.protos.common.FrameProto
import ru.yandex.alice.social.sharing.ApphostHandler
import ru.yandex.alice.social.sharing.apphost.handlers.blackbox.BlackboxResponse
import ru.yandex.alice.social.sharing.apphost.handlers.send_to_device.SendToDeviceHttpRequest
import ru.yandex.alice.social.sharing.proto.context.SendToDevice
import ru.yandex.web.apphost.api.request.RequestContext

abstract class PrepareSendPushRequest(
    protected val objectMapper: ObjectMapper,
): ApphostHandler {

    abstract fun getTypedSemanticFrame(context: RequestContext): FrameProto.TTypedSemanticFrame

    open fun getDeviceId(context: RequestContext): String? {
        val httpRequest = context
            .getSingleRequestItem(PROTO_HTTP_REQUEST)
            .getProtobufData(Http.THttpRequest.getDefaultInstance())
        logger.info("Request body: {}", httpRequest.content.toStringUtf8())
        try {
            val requestBody = objectMapper.readValue(httpRequest.content.toByteArray(), SendToDeviceHttpRequest::class.java)
            return requestBody.deviceId
        } catch (e: Exception) {
            logger.error("Won't send push to device: failed to parse device id from request", e)
            return null
        }
    }

    override fun handle(context: RequestContext) {
        val deviceId = getDeviceId(context)
        if (deviceId == null) {
            return
        }
        val directive = FrameProto.TSemanticFrameRequestData
            .newBuilder()
            .setAnalytics(
                Atm.TAnalyticsTrackingModule.newBuilder()
                    .setOrigin(Atm.TAnalyticsTrackingModule.EOrigin.Web)
                    .setOriginInfo("social_sharing")
                    .setPurpose("start_scenario")
            )
            .setTypedSemanticFrame(getTypedSemanticFrame(context))
            .build()

        val blackboxResponse = BlackboxResponse.fromContext(context, objectMapper)
        if (blackboxResponse == null) {
            logger.error("Won't prepare SendPushRequest: can't parse blackbox response")
            return
        }
        val puid = blackboxResponse.defaultUid
        if (puid == null) {
            logger.error("Won't prepare SendPushRequest: can't find puid in blackbox response")
            return
        }

        context.addProtobufItem(
            SEND_PUSH_REQUEST,
            SendToDevice.TSendPushRequest
                .newBuilder()
                .setUid(puid)
                .setDeviceId(deviceId)
                .setDirective(directive)
                .build()
        )
    }

    companion object {
        private val logger = LogManager.getLogger()
    }
}

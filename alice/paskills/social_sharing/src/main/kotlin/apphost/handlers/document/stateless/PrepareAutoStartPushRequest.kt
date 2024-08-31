package ru.yandex.alice.social.sharing.apphost.handlers.document.stateless

import com.fasterxml.jackson.databind.ObjectMapper
import org.apache.logging.log4j.LogManager
import org.springframework.stereotype.Component
import ru.yandex.alice.megamind.protos.common.FrameProto
import ru.yandex.alice.social.sharing.apphost.handlers.LIST_DEVICES_RESPONSE
import ru.yandex.alice.social.sharing.apphost.handlers.PrepareSendPushRequest
import ru.yandex.alice.social.sharing.document.StatelessDocumentContext
import ru.yandex.alice.social.sharing.document.createExternalSkillTypedSemanticFrame
import ru.yandex.alice.social.sharing.proto.context.ListDevicesProto
import ru.yandex.web.apphost.api.request.RequestContext

@Component
class PrepareAutoStartPushRequest(
    objectMapper: ObjectMapper,
) : PrepareSendPushRequest(objectMapper) {
    override val path = "/stateless_document/autostart/prepare_send_push_request"

    override fun getTypedSemanticFrame(context: RequestContext): FrameProto.TTypedSemanticFrame {
        val params = StatelessDocumentContext.fromContext(context).urlParams
        return createExternalSkillTypedSemanticFrame(params.skillId, params.payload)
    }

    override fun getDeviceId(context: RequestContext): String? {
        val devices = context
            .getSingleRequestItem(LIST_DEVICES_RESPONSE)
            .getProtobufData(ListDevicesProto.TListDevicesResponse.getDefaultInstance())

        if (devices.devicesCount != 1) {
            logger.warn("Won't send push to device: count of devices is ${devices.devicesCount}")
            return null
        }
        return devices.getDevices(0).deviceId
    }

    companion object {
        private val logger = LogManager.getLogger()
    }
}

package ru.yandex.alice.social.sharing.apphost.handlers.send_to_device

import com.fasterxml.jackson.databind.ObjectMapper
import org.apache.logging.log4j.LogManager
import org.springframework.stereotype.Component
import ru.yandex.alice.megamind.protos.common.FrameProto
import ru.yandex.alice.social.sharing.apphost.handlers.DOCUMENT
import ru.yandex.alice.social.sharing.apphost.handlers.PrepareSendPushRequest
import ru.yandex.alice.social.sharing.apphost.handlers.SIGNATURE_VALIDATION_OK
import ru.yandex.alice.social.sharing.apphost.handlers.STATELESS_DOCUMENT_CONTEXT
import ru.yandex.alice.social.sharing.document.StatelessDocumentContext
import ru.yandex.alice.social.sharing.document.createExternalSkillTypedSemanticFrame
import ru.yandex.alice.social.sharing.proto.context.FetchDocumentProto
import ru.yandex.web.apphost.api.request.RequestContext

@Component
class PreparePushRequest(
    objectMapper: ObjectMapper,
): PrepareSendPushRequest(objectMapper) {

    override val path = "/send_to_device/prepare_send_push_request"

    override fun getTypedSemanticFrame(context: RequestContext): FrameProto.TTypedSemanticFrame {
        val documentO = context.getSingleRequestItemO(DOCUMENT)
        val signatureValidationOk = context.getSingleRequestItemO(SIGNATURE_VALIDATION_OK)
        val statelessDocumentContextO = context.getSingleRequestItemO(STATELESS_DOCUMENT_CONTEXT)
        if (documentO.isPresent) {
            logger.info("Creating TypedSemanticFrame from YDB document")
            val document = documentO.get().getProtobufData(FetchDocumentProto.TDocument.getDefaultInstance())
            return document.scenarioSharePage.frame
        } else if (signatureValidationOk.isPresent && statelessDocumentContextO.isPresent) {
            logger.info("Creating TypedSemanticFrame from stateless document")
            val statelessDocumentContext = StatelessDocumentContext.fromContext(context)
            return createExternalSkillTypedSemanticFrame(
                statelessDocumentContext.urlParams.skillId,
                statelessDocumentContext.urlParams.payload
            )
        } else {
            throw RuntimeException("Failed to parse either YDB document or stateless document from context")
        }
    }

    companion object {
        private val logger = LogManager.getLogger()
    }

}

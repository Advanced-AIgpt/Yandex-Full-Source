package ru.yandex.alice.social.sharing.apphost.handlers.document.ydb

import NAppHostHttp.Http
import com.fasterxml.jackson.databind.ObjectMapper
import org.apache.logging.log4j.LogManager
import org.springframework.stereotype.Component
import ru.yandex.alice.megamind.protos.common.FrameProto
import ru.yandex.alice.social.sharing.apphost.handlers.PROTO_HTTP_REQUEST
import ru.yandex.alice.social.sharing.apphost.handlers.PrepareSendPushRequest
import ru.yandex.alice.social.sharing.document.Document
import ru.yandex.alice.social.sharing.document.DocumentProvider
import ru.yandex.alice.social.sharing.document.parseLinkId
import ru.yandex.web.apphost.api.request.RequestContext

private class DocumentRetrievalError(message: String): Exception(message)

@Component
class PrepareYdbDocumentSendPushRequest(
    objectMapper: ObjectMapper,
    private val documentProvider: DocumentProvider,
): PrepareSendPushRequest(objectMapper) {

    override val path = "/ydb_document/prepare_send_push_request"

    override fun getTypedSemanticFrame(context: RequestContext): FrameProto.TTypedSemanticFrame {
        val httpRequest = context
            .getSingleRequestItem(PROTO_HTTP_REQUEST)
            .getProtobufData(Http.THttpRequest.getDefaultInstance())
        val document = findDocument(httpRequest)
        return document.page.frame
    }

    private fun findDocument(httpRequest: Http.THttpRequest): Document {
        val referrer = httpRequest.headersList.firstOrNull { it.name.toLowerCase() == "referer" }?.value
        if (referrer == null) {
            throw DocumentRetrievalError("Can't parse document id from Referer header: header is missing")
        }
        val documentId = parseLinkId(referrer)
        if (documentId == null) {
            throw DocumentRetrievalError("Can't parse document id from Referer header: invalid header value")
        }
        val document = documentProvider.get(documentId)
        if (document == null) {
            throw DocumentRetrievalError("Failed to find document in YDB (documentID = $documentId)")
        }
        return document
    }

    companion object {
        private val logger = LogManager.getLogger()
    }

}

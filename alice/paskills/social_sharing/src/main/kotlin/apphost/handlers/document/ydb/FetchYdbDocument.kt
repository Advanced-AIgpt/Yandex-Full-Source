package ru.yandex.alice.social.sharing.apphost.handlers.document.ydb

import NAppHostHttp.Http
import org.apache.logging.log4j.LogManager
import org.springframework.stereotype.Component
import ru.yandex.alice.social.sharing.ApphostHandler
import ru.yandex.alice.social.sharing.apphost.handlers.DOCUMENT
import ru.yandex.alice.social.sharing.apphost.handlers.PROTO_HTTP_REQUEST
import ru.yandex.alice.social.sharing.apphost.handlers.getHeader
import ru.yandex.alice.social.sharing.document.DocumentProvider
import ru.yandex.alice.social.sharing.proto.context.FetchDocumentProto
import ru.yandex.web.apphost.api.request.RequestContext

abstract class FetchYdbDocument(protected val documentProvider: DocumentProvider): ApphostHandler {
    abstract fun parseDocumentId(request: Http.THttpRequest): String?

    override fun handle(context: RequestContext) {
        val httpRequest = context.getSingleRequestItem(PROTO_HTTP_REQUEST)
            .getProtobufData(Http.THttpRequest.getDefaultInstance())
        val documentId = parseDocumentId(httpRequest)
        logger.info("Trying to fetch document by id: {}", documentId)
        if (documentId != null && isValidDocumentId(documentId)) {
            val document = documentProvider.get(documentId)
            if (document == null) {
                logger.warn("Failed to find document by id: {}", documentId)
                context.addFlag("document_not_found")
                return
            }
            context.addProtobufItem(
                DOCUMENT,
                FetchDocumentProto.TDocument.newBuilder()
                    .setDocumentId(documentId)
                    .setScenarioSharePage(document.page)
                    .build()
            )
        } else {
            logger.error("Document id has invalid format: {}", documentId)
        }
    }

    protected fun getLastPathElement(pathWithQueryString: String): String {
        val path = pathWithQueryString.split("?", limit=2).first()
        val parts = path.split("/")
        val last = parts.last()
        return if (last.isEmpty() && parts.size >= 2) {
            parts[parts.size - 2]
        } else {
            last
        }
    }

    companion object {
        private val logger = LogManager.getLogger()
    }
}

@Component
class FetchYdbDocumentFromPath(
    documentProvider: DocumentProvider,
): FetchYdbDocument(documentProvider) {
    override val path = "/document/fetch"

    override fun parseDocumentId(request: Http.THttpRequest): String {
        return getLastPathElement(request.path)
    }

}

@Component
class FetchYdbDocumentFromReferer(
    documentProvider: DocumentProvider,
): FetchYdbDocument(documentProvider) {

    override fun parseDocumentId(request: Http.THttpRequest): String? {
        val referer = request.headersList.getHeader("Referer")?.value
        if (referer == null) {
            return null
        }
        return getLastPathElement(referer)
    }

    override val path = "/document/fetch_from_referer"
}

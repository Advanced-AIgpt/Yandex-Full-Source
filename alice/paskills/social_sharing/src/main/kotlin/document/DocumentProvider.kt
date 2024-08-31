package ru.yandex.alice.social.sharing.document

import NAppHostHttp.Http
import ru.yandex.alice.social.sharing.apphost.handlers.PROTO_HTTP_REQUEST
import ru.yandex.web.apphost.api.request.RequestContext

abstract class DocumentProvider {
    abstract fun get(id: String): Document?
    protected abstract fun upsertDocument(document: Document)
    abstract fun createCandidate(document: Document)
    abstract fun commitCandidate(id: String)


    open fun getFromContext(context: RequestContext): Document? {
        val request = context.getSingleRequestItem(PROTO_HTTP_REQUEST)
            .getProtobufData(Http.THttpRequest.getDefaultInstance())
        val linkId = parseLinkId(request.path)
        return if (linkId != null) get(linkId) else null
    }
}

class CandidateNotFoundException(val id: String): Exception("Candidate not found: $id")


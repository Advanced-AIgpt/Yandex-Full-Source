package apphost.handlers.document.ydb

import ru.yandex.alice.social.sharing.ApphostHandler
import ru.yandex.alice.social.sharing.apphost.handlers.ParseException
import ru.yandex.alice.social.sharing.apphost.handlers.parse
import ru.yandex.alice.social.sharing.proto.SocialSharingApi
import ru.yandex.web.apphost.api.request.RequestContext


private const val CONTEXT_CREATE_DOCUMENT_REQUEST = "create_document_request";


class ValidateUpsertDocumentRequest : ApphostHandler {

    override val path = "/upsert_document/validate_request"

    override fun handle(context: RequestContext) {
        val request: SocialSharingApi.TCreateAndCommitRequest
        try {
            request = parse(
                context,
                SocialSharingApi.TCreateAndCommitRequest.parser(),
                SocialSharingApi.TCreateAndCommitRequest.newBuilder()
            )
        } catch (e: ParseException) {
            // TODO: write 400
            return
        }
        // TODO: validate required fields
        context.addProtobufItem(CONTEXT_CREATE_DOCUMENT_REQUEST, request)
    }

}

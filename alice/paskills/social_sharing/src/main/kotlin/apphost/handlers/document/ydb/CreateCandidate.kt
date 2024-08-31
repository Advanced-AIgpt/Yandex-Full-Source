package ru.yandex.alice.social.sharing.apphost.handlers.document.ydb

import org.apache.logging.log4j.LogManager
import org.springframework.beans.factory.annotation.Value
import org.springframework.stereotype.Component
import org.springframework.web.util.UriComponentsBuilder
import ru.yandex.alice.social.sharing.ApphostHandler
import ru.yandex.alice.social.sharing.apphost.handlers.CREATE_CANDIDATE_REQUEST
import ru.yandex.alice.social.sharing.apphost.handlers.CREATE_CANDIDATE_RESPONSE
import ru.yandex.alice.social.sharing.document.Document
import ru.yandex.alice.social.sharing.document.DocumentProvider
import ru.yandex.alice.social.sharing.proto.SocialSharingApi
import ru.yandex.web.apphost.api.request.RequestContext

@Component
class CreateCandidate(
    private val documentProvider: DocumentProvider,
    @Value("\${document.ydb.base_url}") private val baseUrl: String,
): ApphostHandler {

    override val path = "/document/create_candidate"

    override fun handle(context: RequestContext) {
        val request = context
            .getSingleRequestItem(CREATE_CANDIDATE_REQUEST)
            .getProtobufData(SocialSharingApi.TCreateCandidateRequest.getDefaultInstance())
        val documentId = generateDocumentId(request)
        val document = Document(documentId, request.createSocialLinkDirective.scenarioSharePage)
        documentProvider.createCandidate(document)
        val url = UriComponentsBuilder
            .fromHttpUrl(baseUrl)
            .pathSegment(documentId)
            .build()
            .toUriString()
        context.addProtobufItem(
            CREATE_CANDIDATE_RESPONSE,
            SocialSharingApi.TCreateCandidateResponse.newBuilder()
                .setLink(
                    SocialSharingApi.TSocialLink.newBuilder()
                        .setId(documentId)
                        .setUrl(url)
                )
                .build()
        )
        logger.info("Created document candidate {}", documentId)
    }

    companion object {
        private val logger = LogManager.getLogger()
    }
}

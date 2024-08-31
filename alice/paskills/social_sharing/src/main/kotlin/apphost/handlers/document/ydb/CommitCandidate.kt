package ru.yandex.alice.social.sharing.apphost.handlers.document.ydb

import org.apache.logging.log4j.LogManager
import org.springframework.stereotype.Component
import ru.yandex.alice.social.sharing.ApphostHandler
import ru.yandex.alice.social.sharing.apphost.handlers.COMMIT_CANDIDATE_REQUEST
import ru.yandex.alice.social.sharing.apphost.handlers.COMMIT_CANDIDATE_RESPONSE
import ru.yandex.alice.social.sharing.document.CandidateNotFoundException
import ru.yandex.alice.social.sharing.document.DocumentProvider
import ru.yandex.alice.social.sharing.proto.SocialSharingApi
import ru.yandex.web.apphost.api.request.RequestContext

@Component
class CommitCandidate(
    private val documentProvider: DocumentProvider,
): ApphostHandler {

    override val path = "/document/commit_candidate"

    override fun handle(context: RequestContext) {
        val request = context
            .getSingleRequestItem(COMMIT_CANDIDATE_REQUEST)
            .getProtobufData(SocialSharingApi.TCommitCandidateRequest.getDefaultInstance())
        val documentId: String = request.candidateId
        val response = SocialSharingApi.TCommitCandidateResponse.newBuilder()
        try {
            documentProvider.commitCandidate(documentId)
            response.setOk(SocialSharingApi.TCommitCandidateResponse.TOk.newBuilder().build())
            logger.info("Successfully committed document {}", documentId)
        } catch (e: CandidateNotFoundException) {
            response.setError(
                SocialSharingApi.TError.newBuilder()
                    .setCode("CANDIDATE_NOT_FOUND")
                    .setMessage("Candidate with id ${documentId} not found")
            )
            logger.warn("Failed to find document candidate in YDB: {}", documentId)
        }
        context.addProtobufItem(COMMIT_CANDIDATE_RESPONSE, response.build())
    }

    companion object {
        private val logger = LogManager.getLogger()
    }

}

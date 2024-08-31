package ru.yandex.alice.social.sharing.apphost.handlers.document.ydb

import org.junit.jupiter.api.Assertions
import org.junit.jupiter.api.BeforeEach
import org.junit.jupiter.api.Test
import ru.yandex.alice.social.sharing.MockRequestContext
import ru.yandex.alice.social.sharing.document.Document
import ru.yandex.alice.social.sharing.document.DocumentProvider
import ru.yandex.alice.social.sharing.document.MockDocumentProvider
import ru.yandex.alice.social.sharing.proto.SocialSharingApi
import ru.yandex.alice.social.sharing.proto.WebPageProto

class CommitCandidateTest {

    private lateinit var documentProvider: DocumentProvider
    private lateinit var handler: CommitCandidate

    @BeforeEach
    fun setUp() {
        documentProvider = MockDocumentProvider()
        handler = CommitCandidate(documentProvider)
    }

    fun commitCandidate(request: SocialSharingApi.TCommitCandidateRequest): SocialSharingApi.TCommitCandidateResponse {
        val context = MockRequestContext("commit_candidate_request" to request)
        handler.handle(context)
        Assertions.assertEquals(context.getOutputItems().size, 1)
        return context.getOutputItemProto(
            "commit_candidate_response",
            SocialSharingApi.TCommitCandidateResponse.getDefaultInstance())
    }

    @Test
    fun commitNonexistentDocument() {
        val response = commitCandidate(
            SocialSharingApi.TCommitCandidateRequest.newBuilder()
                .setCandidateId("123")
                .build()
        )
        Assertions.assertEquals(response.error.code, "CANDIDATE_NOT_FOUND")
    }

    @Test
    fun commitDocument() {
        val candidate = Document(
            "123",
            WebPageProto.TScenarioSharePage.newBuilder().build()
        )
        documentProvider.createCandidate(candidate)
        val response = commitCandidate(
            SocialSharingApi.TCommitCandidateRequest.newBuilder()
                .setCandidateId("123")
                .build()
        )
        Assertions.assertEquals(
            SocialSharingApi.TCommitCandidateResponse.newBuilder()
                .setOk(
                    SocialSharingApi.TCommitCandidateResponse.TOk.newBuilder().build()
                )
                .build(),
            response
        )
    }
}

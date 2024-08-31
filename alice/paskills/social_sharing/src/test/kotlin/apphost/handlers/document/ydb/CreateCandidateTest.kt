package ru.yandex.alice.social.sharing.apphost.handlers.document.ydb

import org.junit.jupiter.api.Assertions
import org.junit.jupiter.api.BeforeEach
import org.junit.jupiter.api.Test
import ru.yandex.alice.social.sharing.MockRequestContext
import ru.yandex.alice.social.sharing.document.MockDocumentProvider
import ru.yandex.alice.social.sharing.proto.DirectivesProto
import ru.yandex.alice.social.sharing.proto.SocialSharingApi
import ru.yandex.alice.social.sharing.proto.WebPageProto

class CreateCandidateTest {

    private lateinit var documentProvider: MockDocumentProvider
    private lateinit var handler: CreateCandidate

    @BeforeEach
    fun setUp() {
        documentProvider = MockDocumentProvider()
        handler = CreateCandidate(
            documentProvider,
            "https://dialogs.yandex.ru/sharing/doc",
        )
    }

    fun handleRequest(request: SocialSharingApi.TCreateCandidateRequest): SocialSharingApi.TCreateCandidateResponse {
        val context = MockRequestContext("create_candidate_request" to request)
        handler.handle(context)
        Assertions.assertEquals(context.getOutputItems().size, 1)
        return context.getOutputItemProto(
            "create_candidate_response",
            SocialSharingApi.TCreateCandidateResponse.getDefaultInstance())
    }

    @Test
    fun testCreateEmptyDocument() {
        val response = handleRequest(SocialSharingApi.TCreateCandidateRequest.newBuilder().build())
        Assertions.assertEquals(
            response,
            SocialSharingApi.TCreateCandidateResponse.newBuilder()
                .setLink(
                    SocialSharingApi.TSocialLink.newBuilder()
                        .setId("QDkoyrjRRliUnx")
                        .setUrl("https://dialogs.yandex.ru/sharing/doc/QDkoyrjRRliUnx")
                )
                .build()
        )
    }

    @Test
    fun testRandomSeedChangesLink() {
        val response1 = handleRequest(SocialSharingApi.TCreateCandidateRequest.newBuilder()
            .setRandomSeed(1)
            .build())
        val response2 = handleRequest(SocialSharingApi.TCreateCandidateRequest.newBuilder()
            .setRandomSeed(2)
            .build())

        Assertions.assertNotEquals(response1.link.id, response2.link.id)
        Assertions.assertNotEquals(response1.link.url, response2.link.url)
    }

    @Test
    fun testPageContentChangesLink() {
        val response1 = handleRequest(SocialSharingApi.TCreateCandidateRequest.newBuilder()
            .setRandomSeed(1)
            .setCreateSocialLinkDirective(
                DirectivesProto.TCreateSocialLinkDirective.newBuilder()
                    .setScenarioSharePage(
                        WebPageProto.TScenarioSharePage.newBuilder()
                            .setFairyTaleTemplate(
                                WebPageProto.TGenerativeFairyTaleTemplate.newBuilder()
                                    .setPageContent(
                                        WebPageProto.TGenerativeFairyTaleTemplate.TPageContent.newBuilder()
                                            .setText("first fairy tale")
                                    )
                            )
                    )
            )
            .build())
        val response2 = handleRequest(SocialSharingApi.TCreateCandidateRequest.newBuilder()
            .setRandomSeed(1)
            .setCreateSocialLinkDirective(
                DirectivesProto.TCreateSocialLinkDirective.newBuilder()
                    .setScenarioSharePage(
                        WebPageProto.TScenarioSharePage.newBuilder()
                            .setFairyTaleTemplate(
                                WebPageProto.TGenerativeFairyTaleTemplate.newBuilder()
                                    .setPageContent(
                                        WebPageProto.TGenerativeFairyTaleTemplate.TPageContent.newBuilder()
                                            .setText("second fairy tale")
                                    )
                            )
                    )
            )
            .build())
        Assertions.assertNotEquals(response1.link.id, response2.link.id)
        Assertions.assertNotEquals(response1.link.url, response2.link.url)
    }

    @Test
    fun testSameRequestProducesSameLink() {
        val requestData = SocialSharingApi.TCreateCandidateRequest.newBuilder()
            .setRandomSeed(1)
            .setCreateSocialLinkDirective(
                DirectivesProto.TCreateSocialLinkDirective.newBuilder()
                    .setScenarioSharePage(
                        WebPageProto.TScenarioSharePage.newBuilder()
                            .setFairyTaleTemplate(
                                WebPageProto.TGenerativeFairyTaleTemplate.newBuilder()
                                    .setPageContent(
                                        WebPageProto.TGenerativeFairyTaleTemplate.TPageContent.newBuilder()
                                            .setText("first fairy tale")
                                    )
                            )
                    )
            )
            .build()
        val response1 = handleRequest(requestData)
        val response2 = handleRequest(requestData)
        Assertions.assertEquals(response1, response2)
    }

}

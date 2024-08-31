package ru.yandex.alice.social.sharing.demoskill

import NAppHostHttp.Http
import com.fasterxml.jackson.databind.JsonNode
import com.fasterxml.jackson.databind.ObjectMapper
import com.fasterxml.jackson.databind.node.ObjectNode
import com.fasterxml.jackson.databind.node.TextNode
import com.google.protobuf.ByteString
import org.springframework.stereotype.Component
import ru.yandex.alice.megamind.protos.common.FrameProto
import ru.yandex.alice.social.sharing.ApphostHandler
import ru.yandex.alice.social.sharing.apphost.handlers.*
import ru.yandex.alice.social.sharing.proto.DirectivesProto
import ru.yandex.alice.social.sharing.proto.SocialSharingApi
import ru.yandex.alice.social.sharing.proto.WebPageProto
import ru.yandex.web.apphost.api.request.RequestContext

@Component
class PrepareCreateCandidateRequest : ApphostHandler {
    override val path = "/skill/prepare_create_candidate_request"

    internal val title = "Игра в города"
    internal val skillId = "672f7477-d3f0-443d-9bd5-2487ab0b6a4c"
    internal val text =
        "В игре Города вы загадываете Город на букву на которую заканчивается Город который придумала Алиса"

    override fun handle(context: RequestContext) {
        context.addProtobufItem(
            CREATE_CANDIDATE_REQUEST,
            SocialSharingApi.TCreateCandidateRequest.newBuilder()
                .setRandomSeed(42)
                .setCreateSocialLinkDirective(
                    DirectivesProto.TCreateSocialLinkDirective.newBuilder()
                        .setScenarioSharePage(
                            WebPageProto.TScenarioSharePage.newBuilder()
                                .setFairyTaleTemplate(
                                    WebPageProto.TGenerativeFairyTaleTemplate.newBuilder()
                                        .setPageContent(
                                            WebPageProto.TGenerativeFairyTaleTemplate.TPageContent.newBuilder()
                                                .setText(text)
                                                .setTitle(title)
                                                .setBackgroundColor("#FFFFFF")
                                                .setCardColor("#F2F3F5")
                                                .build()
                                        )
                                        .build()
                                ).setSocialNetworkMarkup(
                                    WebPageProto.TScenarioSharePage.TSocialNetworkMarkup.newBuilder()
                                        .setTitle(title)
                                        .setDescription(text)
                                        .setType("article")
                                        .build()
                                ).setFrame(
                                    FrameProto.TTypedSemanticFrame.newBuilder()
                                        .setExternalSkillFixedActivateSemanticFrame(
                                            FrameProto.TExternalSkillFixedActivateSemanticFrame.newBuilder()
                                                .setFixedSkillId(
                                                    FrameProto.TStringSlot.newBuilder()
                                                        .setStringValue(skillId)
                                                        .build()
                                                )
                                                .build()
                                        )
                                        .build()
                                )
                        )
                )
                .build()
        )
    }
}

@Component
class PrepareCommitCandidateRequest : ApphostHandler {
    override val path = "/skill/prepare_commit_candidate_request"

    override fun handle(context: RequestContext) {
        val createCandidateResponse = context.getSingleRequestItem(CREATE_CANDIDATE_RESPONSE)
            .getProtobufData(SocialSharingApi.TCreateCandidateResponse.getDefaultInstance())
        val documentId = createCandidateResponse.link.id
        context.addProtobufItem(
            COMMIT_CANDIDATE_REQUEST,
            SocialSharingApi.TCommitCandidateRequest
                .newBuilder()
                .setCandidateId(documentId)
                .build()
        )
    }
}

@Component
class RenderResponse(
    private val objectMapper: ObjectMapper,
) : ApphostHandler {
    override val path = "/skill/render_response"

    override fun handle(context: RequestContext) {
        val createCandidateResponse = context.getSingleRequestItem(CREATE_CANDIDATE_RESPONSE)
            .getProtobufData(SocialSharingApi.TCreateCandidateResponse.getDefaultInstance())
        val commitCandidateResponse = context.getSingleRequestItemO(COMMIT_CANDIDATE_RESPONSE)
        val response: JsonNode = if (commitCandidateResponse.isPresent &&
            commitCandidateResponse.get()
                .getProtobufData(SocialSharingApi.TCommitCandidateResponse.getDefaultInstance())
                .hasOk()
        ) renderSuccess(createCandidateResponse) else renderError()
        context.addProtobufItem(
            PROTO_HTTP_RESPONSE,
            Http.THttpResponse.newBuilder()
                .setStatusCode(200)
                .setContent(ByteString.copyFrom(objectMapper.writeValueAsBytes(response)))
                .addAllHeaders(DEFAULT_HEADERS_JSON)
                .build()
        )
    }

    private fun renderSuccess(createCandidateResponse: SocialSharingApi.TCreateCandidateResponse): JsonNode {
        return renderSkillResponse(createCandidateResponse.link.url)
    }

    private fun renderError(): JsonNode {
        return renderSkillResponse("Ошибка")
    }

    private fun renderSkillResponse(text: String): JsonNode {
        val response = ObjectNode(objectMapper.nodeFactory)
        response.set<ObjectNode>("text", TextNode(text))

        val body = ObjectNode(objectMapper.nodeFactory)
        body.set<ObjectNode>("version", TextNode("1.0"))
        body.set<ObjectNode>("response", response)
        return body
    }
}

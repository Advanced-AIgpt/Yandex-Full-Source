package ru.yandex.alice.paskill.dialogovo.processor

import org.apache.logging.log4j.LogManager
import org.apache.logging.log4j.Logger
import org.springframework.beans.factory.annotation.Value
import org.springframework.stereotype.Component
import org.springframework.web.util.UriComponentsBuilder
import ru.yandex.alice.kronstadt.core.AliceHandledException
import ru.yandex.alice.kronstadt.core.RequestContext
import ru.yandex.alice.kronstadt.core.directive.OpenUriDirective
import ru.yandex.alice.kronstadt.core.directive.server.SendPushMessageDirective
import ru.yandex.alice.kronstadt.core.domain.ClientInfo
import ru.yandex.alice.kronstadt.core.domain.Voice
import ru.yandex.alice.kronstadt.core.layout.Button
import ru.yandex.alice.kronstadt.core.layout.TextCard
import ru.yandex.alice.kronstadt.core.layout.TextWithTts
import ru.yandex.alice.kronstadt.core.text.Phrases
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo
import ru.yandex.alice.paskill.dialogovo.domain.SkillProcessRequest
import ru.yandex.alice.paskill.dialogovo.domain.SkillProcessResult
import ru.yandex.alice.paskill.dialogovo.domain.UserAgreement
import ru.yandex.alice.paskill.dialogovo.external.v1.response.WebhookResponse
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context
import ru.yandex.alice.paskill.dialogovo.processor.request_enrichment.RequestEnrichmentData
import ru.yandex.alice.paskill.dialogovo.providers.skill.UserAgreementProvider
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.ErrorAnalyticsInfoAction
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.directives.UserAgreementsAcceptedDirective
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.directives.UserAgreementsRejectedDirective
import ru.yandex.alice.paskill.dialogovo.utils.TextUtils
import java.net.URI
import java.util.UUID

@Component
class ShowUserAgreementsHandler(
    private val phrases: Phrases,
    private val userAgreementProvider: UserAgreementProvider,
    private val directiveConverter: DirectiveToDialogUriConverter,
    private val requestContext: RequestContext,
    @Value("\${storeUrl}") private val storeUrl: String,
) : WebhookResponseHandler {

    override fun handleResponse(
        builder: SkillProcessResult.Builder,
        request: SkillProcessRequest,
        context: Context,
        requestEnrichment: RequestEnrichmentData,
        response: WebhookResponse
    ): SkillProcessResult.Builder {
        val clientInfo = request.clientInfo
        if (!clientInfo.isSupportUserAgreements) {
            val reply = phrases.getRandomTextWithTts("user_agreements.not_supported", request.random)
            builder.getLayout().cards = mutableListOf(TextCard(reply.text))
            builder.setTts(reply.tts)
            return builder
        }
        val skillId: UUID = UUID.fromString(request.skill.id)
        val userAgreements = if (request.skill.draft)
            userAgreementProvider.getDraftUserAgreements(skillId)
        else userAgreementProvider.getPublishedUserAgreements(skillId)
        if (userAgreements.isEmpty()) {
            logger.info("No user agreements found")
            return builder
        }
        val link: URI = generateUserAgreementListLink(
            request.clientInfo,
            request.skill,
            userAgreements,
            request.isVoiceSession
        )
        // If user try to open this link outside ya app, deep-links won't work
        // In that case ya app will be opened first
        val yaAppOpeningLink =
            "ya-search-app-open://path?uri=${TextUtils.urlEncode(link.toString())}"

        if (clientInfo.isYaBrowser || clientInfo.isSearchApp || clientInfo.isDevConsole) {
            val reply: TextWithTts = phrases.getRandomTextWithTts(
                if (userAgreements.size == 1) "user_agreements.show.single" else "user_agreements.show.multiple",
                request.random,
                request.skill.nameAsTextWithTts
            )
            val buttons = listOf(
                Button.simpleText(
                    phrases.getRandom("user_agreements.button.open", request.random),
                    OpenUriDirective(yaAppOpeningLink)
                ),
            )
            builder.getLayout()
                .textCard(reply.text, buttons)
            builder.appendTts(reply.tts, Voice.SHITOVA_US)
            return builder
        } else {
            // station-like surfaces
            // convert existing text cards to builder to append Alice's reply
            val textBuilder = TextCard.Builder()
            val existingText: TextCard? = (builder.getLayout().cards.firstOrNull { it is TextCard }) as TextCard?
            if (existingText != null) {
                textBuilder.appendText(existingText.text)
                textBuilder.buttons(existingText.buttons)
            }
            val reply: TextWithTts = phrases.getRandomTextWithTts(
                if (userAgreements.size == 1) "user_agreements.show.single.push" else "user_agreements.show.multiple.push",
                request.random,
                request.skill.nameAsTextWithTts
            )
            textBuilder.appendText(reply.text)
            builder.getLayout().cards = mutableListOf(textBuilder.build())
            builder.appendTts(reply.tts, Voice.SHITOVA_US)
            if (requestContext.currentUserId != null && context.source.isByUser) {
                val title = phrases.getRandom("user_agreements.push.title", request.random, request.skill.name)
                builder.serverDirective(
                    SendPushMessageDirective(
                        title = title,
                        body = phrases.getRandom("user_agreements.push.body", request.random),
                        link = URI(yaAppOpeningLink),
                        pushId = "alice_skill_user_agreement",
                        pushTag = "alice-skill-show-user-agreement",
                        throttlePolicy = SendPushMessageDirective.ALICE_DEFAULT_DEVICE_ID,
                        appTypes = listOf(SendPushMessageDirective.AppType.SEARCH_APP),
                        cardTitle = title,
                        cardButtonText = "Принять пользовательское соглашение",
                    )
                )
            } else {
                logger.error(
                    "Cannot append push text directive, currentUserId = {}, source = {}",
                    requestContext.currentUserId,
                    context.source.isByUser,
                )
            }
            return builder
        }
    }

    private fun generateUserAgreementListLink(
        clientInfo: ClientInfo,
        skill: SkillInfo,
        userAgreements: List<UserAgreement>,
        isVoiceSession: Boolean
    ): URI {
        val initialDeviceId: String?
        if (clientInfo.isYaSmartDevice) {
            if (clientInfo.deviceId != null) {
                initialDeviceId = clientInfo.deviceId
            } else {
                throw AliceHandledException(
                    "Missing device id",
                    ErrorAnalyticsInfoAction.USER_AGREEMENT_LIST_FAILURE,
                    "Не удалось открыть список пользовательских соглашений.",
                    "Не удалось открыть список пользовательских соглашений."
                )
            }
        } else {
            initialDeviceId = null
        }
        return UriComponentsBuilder.fromUriString(storeUrl)
            .pathSegment("skills", skill.slug, "user-agreements")
            .queryParam("session_type", if (isVoiceSession) "voice" else "text")
            .queryParam(
                "on_agree",
                TextUtils.urlEncode(
                    directiveConverter.convertDirectives(
                        listOf(
                            UserAgreementsAcceptedDirective(
                                skill.id,
                                userAgreements.map { it.id.toString() },
                                initialDeviceId
                            )
                        ),
                        skill.id
                    ).toString()
                )
            )
            .queryParam(
                "on_reject",
                TextUtils.urlEncode(
                    directiveConverter.convertDirectives(
                        listOf(
                            UserAgreementsRejectedDirective(skill.id, initialDeviceId)
                        ),
                        skill.id
                    ).toString()
                )
            )
            .build(true).toUri()
    }

    companion object {
        val logger: Logger = LogManager.getLogger()
    }
}

package ru.yandex.alice.kronstadt.scenarios.video_call.scenes

import org.apache.logging.log4j.LogManager
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.ActionSpace
import ru.yandex.alice.kronstadt.core.DivRenderData
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.RelevantResponse
import ru.yandex.alice.kronstadt.core.RunOnlyResponse
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfo
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoContext
import ru.yandex.alice.kronstadt.core.directive.ShowViewDirective
import ru.yandex.alice.kronstadt.core.domain.Contact
import ru.yandex.alice.kronstadt.core.layout.Layout
import ru.yandex.alice.kronstadt.core.layout.TextWithTts
import ru.yandex.alice.kronstadt.core.scenario.AbstractScene
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticFrameRequestData
import ru.yandex.alice.kronstadt.scenarios.video_call.CALL_TO_INTENT
import ru.yandex.alice.kronstadt.scenarios.video_call.MESSENGER_HANGUP_CALL_FRAME
import ru.yandex.alice.kronstadt.scenarios.video_call.VIDEO_CALL
import ru.yandex.alice.kronstadt.scenarios.video_call.analytics.MatchedContactsObject
import ru.yandex.alice.kronstadt.scenarios.video_call.analytics.VideoCallToAction
import ru.yandex.alice.kronstadt.scenarios.video_call.cards.BIND_TO_CALL_VARIABLE
import ru.yandex.alice.kronstadt.scenarios.video_call.cards.CustomTelegramDiv2Card
import ru.yandex.alice.kronstadt.scenarios.video_call.domain.OutgoingTelegramCallScenarioData
import ru.yandex.alice.kronstadt.scenarios.video_call.domain.ProviderContactData
import ru.yandex.alice.kronstadt.scenarios.video_call.domain.StartVideoCallDirective
import ru.yandex.alice.kronstadt.scenarios.video_call.domain.VideoCallOutgoingAcceptedSemanticFrame
import ru.yandex.alice.kronstadt.scenarios.video_call.domain.VideoCallOutgoingFailedSemanticFrame
import ru.yandex.alice.kronstadt.scenarios.video_call.domain.converter.MessengerCallHangupSemanticFrame
import ru.yandex.alice.kronstadt.scenarios.video_call.utils.VideoCallCapabilityExtension.getTelegramUserId
import ru.yandex.alice.kronstadt.scenarios.video_call.utils.VideoCallCapabilityExtension.hasActiveCall
import ru.yandex.alice.kronstadt.scenarios.video_call.domain.converter.VideoCallScenarioDataConverter
import java.util.UUID

private const val OUTGOING_CALL_CARD_ID = "video_call.outgoing_call.card.id"

private const val VIDEO_CALL_CONST_START_VIDEO_CALL_ID = "test_video_call_const_start_video_call_id"
private const val CONST_START_VIDEO_CALL_ID = "start_video_call_id"

private const val HANGUP_CALL_ACTION_ID = "video_call.current_call.hangup.action.id"

private const val MOCKED_VIDEO_CALL_START_EXP = "mocked_video_call_start"

const val VIDEO_CALL_RENDER_DATA_EXP = "video_call_render_data_exp"
const val CURRENT_CALL_ACTION_SPACE_ID = "video_call.current_call.action_space_id"

@Component
class ActivateVideoCallScene(
    private val scenarioDataConverter: VideoCallScenarioDataConverter
) : AbstractScene<Any, ActivateVideoCallScene.Args>(
        name = "activate_video_call_scene",
        argsClass = Args::class
) {
    private val logger = LogManager.getLogger(ActivateVideoCallScene::class.java)

    data class Args(
        val userId: String,
        val recipient: ProviderContactData,
        val matchedContact: Contact,
        val videoEnabled: Boolean = true,
    )

    override fun render(request: MegaMindRequest<Any>, args: Args): RelevantResponse<Any> {
        if (request.videoCallCapability?.hasActiveCall() == true) {
            throw IllegalStateException("Already have active telegram call in videoCall state, only one call allowed")
        }
        val startVideoCallId = if (!request.hasExperiment(VIDEO_CALL_CONST_START_VIDEO_CALL_ID))
            UUID.randomUUID().toString() else CONST_START_VIDEO_CALL_ID
        val recipientUserId = args.recipient.userId
        logger.info("Calling to telegram id: $recipientUserId...")
        return RunOnlyResponse(
            layout = Layout.textWithOutputSpeech(
                shouldListen = false,
                textWithTts = TextWithTts("${args.recipient.displayName}, уже набираю"),
                directives = if (!request.hasExperiment(MOCKED_VIDEO_CALL_START_EXP)) {
                    listOf(
                        getStartVideoCallDirective(recipientUserId, startVideoCallId, request, args),
                        getStartCallShowView(startVideoCallId, !request.hasExperiment(VIDEO_CALL_RENDER_DATA_EXP))
                    )
                } else listOf(getStartCallShowView(startVideoCallId, !request.hasExperiment(VIDEO_CALL_RENDER_DATA_EXP)))
            ),
            state = null,
            analyticsInfo = createAnalyticsInfo(args),
            isExpectsRequest = false,
            actionSpaces = mapOf(CURRENT_CALL_ACTION_SPACE_ID to currentCallSpaceAction()),
            renderData = if (request.hasExperiment(VIDEO_CALL_RENDER_DATA_EXP)) listOf(
                DivRenderData(
                    cardId = OUTGOING_CALL_CARD_ID,
                    scenarioData = getScenarioData(args)
                )
            ) else listOf(),
        )
    }

    private fun getScenarioData(args: Args) = scenarioDataConverter.convert(
        OutgoingTelegramCallScenarioData(userId = args.userId, recipient = args.recipient),
        ToProtoContext()
    )

    private fun createAnalyticsInfo(args: Args) = AnalyticsInfo(
        intent = CALL_TO_INTENT,
        objects = listOf(MatchedContactsObject(listOf(args.matchedContact))),
        actions = listOf(VideoCallToAction(args.matchedContact)),
    )

    private fun getStartVideoCallDirective(
        recipientUserId: String,
        startVideoCallId: String,
        request: MegaMindRequest<Any>,
        args: Args
    ): StartVideoCallDirective {
        val onAcceptedSemanticFrame = VideoCallOutgoingAcceptedSemanticFrame
        val onAcceptedCallback = SemanticFrameRequestData(
            onAcceptedSemanticFrame,
            SemanticFrameRequestData.AnalyticsTrackingModule(
                productScenario = VIDEO_CALL.productScenarioName,
                purpose = onAcceptedSemanticFrame.defaultPurpose()
            )
        )

        val onFailedSemanticFrame = VideoCallOutgoingFailedSemanticFrame
        val onFailedCallback = SemanticFrameRequestData(
            onFailedSemanticFrame,
            SemanticFrameRequestData.AnalyticsTrackingModule(
                productScenario = VIDEO_CALL.productScenarioName,
                purpose = onFailedSemanticFrame.defaultPurpose()
            )
        )

        return StartVideoCallDirective(
            providerData = StartVideoCallDirective.TelegramStartVideoCallData(
                id = startVideoCallId,
                userId = request.videoCallCapability?.getTelegramUserId()
                    ?: throw IllegalStateException("Telegram user id must be provided"),
                recipientUserId = recipientUserId,
                videoEnabled = args.videoEnabled,
                onAcceptedCallback = onAcceptedCallback,
                onFailCallback = onFailedCallback
            )
        )
    }

    private fun getStartCallShowView(startVideoCallId: String, withDiv2Card: Boolean) = ShowViewDirective(
        layer = ShowViewDirective.Layer.CONTENT,
        card = if (withDiv2Card) ShowViewDirective.Div2Card(CustomTelegramDiv2Card.card(
            logId = "telegram_start_call_screen",
            customType = "telegram_start_call",
            customProps = mapOf("id" to startVideoCallId),
            extension = "telegram-start-call",
            variable = BIND_TO_CALL_VARIABLE,
        )) else ShowViewDirective.CardId(OUTGOING_CALL_CARD_ID),
        inactivityTimeout = ShowViewDirective.InactivityTimeout.SHORT,
        actionSpaceId = CURRENT_CALL_ACTION_SPACE_ID,
    )
}

fun currentCallSpaceAction() = ActionSpace(
    effects = mapOf(
        HANGUP_CALL_ACTION_ID to ActionSpace.TypedSemanticFrameEffect(
            SemanticFrameRequestData(
                MessengerCallHangupSemanticFrame,
                SemanticFrameRequestData.AnalyticsTrackingModule(
                    VIDEO_CALL.productScenarioName,
                    MessengerCallHangupSemanticFrame.defaultPurpose()
                ))),
    ),
    nluHints = listOf(
        ActionSpace.NluHint(
            actionId = HANGUP_CALL_ACTION_ID,
            semanticFrameName = MESSENGER_HANGUP_CALL_FRAME
        ),
    ))
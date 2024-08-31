package ru.yandex.alice.kronstadt.scenarios.video_call.scenes

import org.apache.logging.log4j.LogManager
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.DivRenderData
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.RelevantResponse
import ru.yandex.alice.kronstadt.core.RunOnlyResponse
import ru.yandex.alice.kronstadt.core.VideoCallCapability
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfo
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoContext
import ru.yandex.alice.kronstadt.core.directive.HideViewDirective
import ru.yandex.alice.kronstadt.core.directive.ShowViewDirective
import ru.yandex.alice.kronstadt.core.layout.Layout
import ru.yandex.alice.kronstadt.core.scenario.AbstractNoargScene
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticFrameRequestData
import ru.yandex.alice.kronstadt.scenarios.video_call.ACCEPT_CALL_INTENT
import ru.yandex.alice.kronstadt.scenarios.video_call.VIDEO_CALL
import ru.yandex.alice.kronstadt.scenarios.video_call.analytics.AcceptVideoCallAction
import ru.yandex.alice.kronstadt.scenarios.video_call.cards.BIND_TO_CALL_VARIABLE
import ru.yandex.alice.kronstadt.scenarios.video_call.cards.CustomTelegramDiv2Card
import ru.yandex.alice.kronstadt.scenarios.video_call.domain.AcceptVideoCallDirective
import ru.yandex.alice.kronstadt.scenarios.video_call.domain.CurrentTelegramCallScenarioData
import ru.yandex.alice.kronstadt.scenarios.video_call.domain.ProviderContactData
import ru.yandex.alice.kronstadt.scenarios.video_call.domain.VideoCallIncomingAcceptFailedSemanticFrame
import ru.yandex.alice.kronstadt.scenarios.video_call.utils.VideoCallCapabilityExtension.getIncomingTelegramCallData
import ru.yandex.alice.kronstadt.scenarios.video_call.domain.converter.VideoCallScenarioDataConverter
import ru.yandex.alice.kronstadt.scenarios.video_call.utils.VideoCallCapabilityExtension.getTelegramUserId

const val CURRENT_CALL_CARD_ID = "video_call.current_call.card.id"

@Component
class AcceptIncomingCallScene(
    private val scenarioDataConverter: VideoCallScenarioDataConverter
)
    : AbstractNoargScene<Any>(
    name = "accept_incoming_call_scene"
) {
    private val logger = LogManager.getLogger(ActivateVideoCallScene::class.java)

    override fun render(request: MegaMindRequest<Any>): RelevantResponse<Any> {
        val incomingCallData = request.videoCallCapability!!.getIncomingTelegramCallData()
                ?: throw IllegalStateException("Incoming telegram call data not found in videoCall state")

        return RunOnlyResponse(
            layout = Layout.directiveOnlyLayout(
                directives = with(incomingCallData.callOwnerData) {
                    listOf(
                        getAcceptVideoCallDirective(callId, userId),
                        getActiveCallShowView(callId, !request.hasExperiment(VIDEO_CALL_RENDER_DATA_EXP)),
                        HideViewDirective(layer = HideViewDirective.Layer.ALARM),
                    )}),
            state = null,
            analyticsInfo = AnalyticsInfo(
                intent = ACCEPT_CALL_INTENT,
                actions = listOf(AcceptVideoCallAction)
            ),
            isExpectsRequest = false,
            actionSpaces = mapOf(CURRENT_CALL_ACTION_SPACE_ID to currentCallSpaceAction()),
            renderData = if (request.hasExperiment(VIDEO_CALL_RENDER_DATA_EXP)) listOf(
                DivRenderData(
                    cardId = CURRENT_CALL_CARD_ID,
                    scenarioData = getScenarioData(
                        request.videoCallCapability!!.getTelegramUserId()!!,
                        incomingCallData,
                ))
            ) else listOf(),
        )
    }

    private fun getScenarioData(
        userId: String,
        incomingCallData: VideoCallCapability.TelegramCallData,
    ) = scenarioDataConverter.convert(
        CurrentTelegramCallScenarioData(
            userId = userId,
            callId = incomingCallData.callOwnerData.callId,
            recipient = ProviderContactData(incomingCallData.recipient.userId),
        ),
        ToProtoContext()
    )
}

fun getAcceptVideoCallDirective(
    callId: String, userId: String
): AcceptVideoCallDirective {
    val onFailSemanticFrame = VideoCallIncomingAcceptFailedSemanticFrame(callId)
    val onFailCallback =  SemanticFrameRequestData(
        onFailSemanticFrame,
        SemanticFrameRequestData.AnalyticsTrackingModule(
            productScenario = VIDEO_CALL.productScenarioName,
            purpose = onFailSemanticFrame.defaultPurpose()
        )
    )

    return AcceptVideoCallDirective(
        providerData = AcceptVideoCallDirective.TelegramAcceptVideoCallData(
            callOwnerData = VideoCallCapability.TelegramVideoCallOwnerData(
                callId = callId,
                userId = userId
            ),
            onFailCallback = onFailCallback
        )
    )
}

fun getActiveCallShowView(callId: String, withDiv2Card: Boolean) = ShowViewDirective(
    layer = ShowViewDirective.Layer.CONTENT,
    card =  if (withDiv2Card) ShowViewDirective.Div2Card(
        CustomTelegramDiv2Card.card(
            logId = "telegram_call_screen",
            customType = "telegram_call",
            customProps = mapOf("id" to callId),
            extension = "telegram-call",
            variable = BIND_TO_CALL_VARIABLE,
        )) else ShowViewDirective.CardId(CURRENT_CALL_CARD_ID),
    inactivityTimeout = ShowViewDirective.InactivityTimeout.SHORT,
    actionSpaceId = CURRENT_CALL_ACTION_SPACE_ID,
)

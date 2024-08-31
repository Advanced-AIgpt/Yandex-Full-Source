package ru.yandex.alice.kronstadt.scenarios.video_call.scenes

import org.apache.logging.log4j.LogManager
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.ActionRef
import ru.yandex.alice.kronstadt.core.ActionSpace
import ru.yandex.alice.kronstadt.core.DivRenderData
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.RelevantResponse
import ru.yandex.alice.kronstadt.core.RunOnlyResponse
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfo
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoContext
import ru.yandex.alice.kronstadt.core.directive.HideViewDirective
import ru.yandex.alice.kronstadt.core.directive.ShowViewDirective
import ru.yandex.alice.kronstadt.core.directive.TtsPlayPlaceholderDirective
import ru.yandex.alice.kronstadt.core.layout.Layout
import ru.yandex.alice.kronstadt.core.layout.TextWithTts
import ru.yandex.alice.kronstadt.core.scenario.AbstractScene
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticFrameRequestData
import ru.yandex.alice.kronstadt.scenarios.video_call.CALL_INCOMING_INTENT
import ru.yandex.alice.kronstadt.scenarios.video_call.MESSENGER_ACCEPT_INCOMING_CALL_FRAME
import ru.yandex.alice.kronstadt.scenarios.video_call.MESSENGER_DISCARD_INCOMING_CALL_FRAME
import ru.yandex.alice.kronstadt.scenarios.video_call.VIDEO_CALL
import ru.yandex.alice.kronstadt.scenarios.video_call.VIDEO_CALL_INCOMING_FRAME
import ru.yandex.alice.kronstadt.scenarios.video_call.domain.IncomingTelegramCallScenarioData
import ru.yandex.alice.kronstadt.scenarios.video_call.domain.ProviderContactData
import ru.yandex.alice.kronstadt.scenarios.video_call.domain.converter.MessengerCallAcceptSemanticFrame
import ru.yandex.alice.kronstadt.scenarios.video_call.domain.converter.MessengerCallDiscardSemanticFrame
import ru.yandex.alice.kronstadt.scenarios.video_call.domain.converter.VideoCallScenarioDataConverter

const val INCOMING_CALL_CARD_ID = "video_call.incoming_call.card.id"
const val INCOMING_CALL_ACTION_SPACE_ID = "video_call.incoming_call.action_space_id"
const val ACCEPT_CALL_ACTION_ID = "video_call.incoming_call.accept.action.id"
const val DISCARD_CALL_ACTION_ID = "video_call.incoming_call.discard.action.id"
const val INCOMING_CALL_RENDER_DATA_EXP = "incoming_call_render_data_exp"
const val INCOMING_CALL_SPACE_ACTIONS_EXP = "incoming_call_space_actions"

@Component
class IncomingVideoCallScene(
    private val scenarioDataConverter: VideoCallScenarioDataConverter
) : AbstractScene<Any, IncomingVideoCallScene.Args>(
        name = "incoming_video_call_scene",
        argsClass = Args::class
) {
    private val logger = LogManager.getLogger(IncomingVideoCallScene::class.java)

    data class Args(val callId: String, val userId: String, val caller: ProviderContactData?)

    override fun render(request: MegaMindRequest<Any>, args: Args): RelevantResponse<Any> {
        args.caller?.userId?.apply {
            logger.info("Incoming call from telegram id: $this...")
            return RunOnlyResponse(
                layout = Layout.textWithOutputSpeech(
                    shouldListen = false,
                    textWithTts = TextWithTts("Входящий звонок от ${args.caller.displayName}"),
                    directives = listOf(
                        INCOMING_CALL_SHOW_VIEW,
                        TtsPlayPlaceholderDirective.TTS_PLAY_PLACEHOLDER_DIRECTIVE_DIALOG
                    )
                ),
                state = null,
                analyticsInfo = AnalyticsInfo(intent = CALL_INCOMING_INTENT),
                isExpectsRequest = false,
                renderData = if (!request.hasExperiment(INCOMING_CALL_RENDER_DATA_EXP)) listOf(
                    DivRenderData(
                        cardId = INCOMING_CALL_CARD_ID,
                        scenarioData = getScenarioData(args)
                    )
                ) else listOf(),
                actions = if (!request.hasExperiment(INCOMING_CALL_SPACE_ACTIONS_EXP))
                    mapOf(
                        "accept_incoming_call" to acceptCallVoiceButton(request, args),
                        "discard_incoming_call" to discardCallVoiceButton(args),
                    ) else mapOf(),
                actionSpaces = if (request.hasExperiment(INCOMING_CALL_SPACE_ACTIONS_EXP)) mapOf(
                    INCOMING_CALL_ACTION_SPACE_ID to incomingCallSpaceAction(),
                    CURRENT_CALL_ACTION_SPACE_ID to currentCallSpaceAction(),
                ) else mapOf(),
            )
        }
        logger.warn("Couldn't match caller contact for incoming frame: ${request.getSemanticFrame(VIDEO_CALL_INCOMING_FRAME)
            !!.typedSemanticFrame!!.videoCallIncomingSemanticFrame}")
        return RunOnlyResponse(
                layout = Layout.silence(),
                state = null,
                analyticsInfo = AnalyticsInfo(intent = CALL_INCOMING_INTENT)
            )
    }

    private fun acceptCallVoiceButton(request: MegaMindRequest<Any>, args: Args) = ActionRef.withDirectives(
        directives = with(args) {
            listOf(
                getAcceptVideoCallDirective(callId = callId, userId = userId),
                getActiveCallShowView(callId = callId, withDiv2Card = !request.hasExperiment(VIDEO_CALL_RENDER_DATA_EXP)),
                HideViewDirective(layer = HideViewDirective.Layer.ALARM),
            )},
        nluHint = ActionRef.NluHint(frameName = MESSENGER_ACCEPT_INCOMING_CALL_FRAME))

    private fun discardCallVoiceButton(args: Args) = ActionRef.withDirectives(
        directives = listOf(
            getDiscardVideoCallDirective(callId = args.callId, userId = args.userId),
        ),
        nluHint = ActionRef.NluHint(frameName = MESSENGER_DISCARD_INCOMING_CALL_FRAME))

    private fun incomingCallSpaceAction() = ActionSpace(
        effects = mapOf(
            ACCEPT_CALL_ACTION_ID to ActionSpace.TypedSemanticFrameEffect(
                    SemanticFrameRequestData(
                        MessengerCallAcceptSemanticFrame,
                        SemanticFrameRequestData.AnalyticsTrackingModule(
                            VIDEO_CALL.productScenarioName,
                            MessengerCallAcceptSemanticFrame.defaultPurpose()
                        ))),
            DISCARD_CALL_ACTION_ID to ActionSpace.TypedSemanticFrameEffect(
                    SemanticFrameRequestData(
                        MessengerCallDiscardSemanticFrame,
                        SemanticFrameRequestData.AnalyticsTrackingModule(
                            VIDEO_CALL.productScenarioName,
                            MessengerCallDiscardSemanticFrame.defaultPurpose()
                        ))),
        ),
        nluHints = listOf(
            ActionSpace.NluHint(
                actionId = ACCEPT_CALL_ACTION_ID,
                semanticFrameName = MESSENGER_ACCEPT_INCOMING_CALL_FRAME),
            ActionSpace.NluHint(
                actionId = DISCARD_CALL_ACTION_ID,
                semanticFrameName = MESSENGER_DISCARD_INCOMING_CALL_FRAME),
        ))

   private fun getScenarioData(args: Args) = scenarioDataConverter.convert(
       IncomingTelegramCallScenarioData(userId = args.userId, callId = args.callId, caller = args.caller!!),
       ToProtoContext()
   )

    companion object {
        private val INCOMING_CALL_SHOW_VIEW = ShowViewDirective(
            layer = ShowViewDirective.Layer.ALARM,
            card = ShowViewDirective.CardId(INCOMING_CALL_CARD_ID),
            inactivityTimeout = ShowViewDirective.InactivityTimeout.SHORT,
            actionSpaceId = INCOMING_CALL_ACTION_SPACE_ID,
        )
    }
}

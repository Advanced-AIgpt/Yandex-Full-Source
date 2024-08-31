package ru.yandex.alice.kronstadt.scenarios.video_call.scenes

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.DivRenderData
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.RelevantResponse
import ru.yandex.alice.kronstadt.core.RunOnlyResponse
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfo
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoContext
import ru.yandex.alice.kronstadt.core.layout.Layout
import ru.yandex.alice.kronstadt.core.scenario.AbstractScene
import ru.yandex.alice.kronstadt.scenarios.video_call.CALL_TO_INTENT
import ru.yandex.alice.kronstadt.scenarios.video_call.domain.CurrentTelegramCallScenarioData
import ru.yandex.alice.kronstadt.scenarios.video_call.domain.ProviderContactData
import ru.yandex.alice.kronstadt.scenarios.video_call.domain.converter.VideoCallScenarioDataConverter

@Component
class OutgoingAcceptedScene(
    private val scenarioDataConverter: VideoCallScenarioDataConverter
)
    : AbstractScene<Any, OutgoingAcceptedScene.Args>(
    name = "outgoing_accepted_scene",
    argsClass = Args::class
) {

    data class Args(
        val callId: String,
        val userId: String,
        val recipient: ProviderContactData)

    override fun render(request: MegaMindRequest<Any>, args: Args): RelevantResponse<Any> {
        return RunOnlyResponse(
            layout = Layout.directiveOnlyLayout(
                directives = listOf(
                    getActiveCallShowView(args.callId, !request.hasExperiment(VIDEO_CALL_RENDER_DATA_EXP))
                )
            ),
            state = null,
            analyticsInfo = AnalyticsInfo(intent = CALL_TO_INTENT),
            isExpectsRequest = false,
            actionSpaces = mapOf(CURRENT_CALL_ACTION_SPACE_ID to currentCallSpaceAction()),
            renderData = if (request.hasExperiment(VIDEO_CALL_RENDER_DATA_EXP)) listOf(
                DivRenderData(
                    cardId = CURRENT_CALL_CARD_ID,
                    scenarioData = getScenarioData(args)
                )
            ) else listOf(),
        )
    }

    private fun getScenarioData(args: Args) = scenarioDataConverter.convert(
        CurrentTelegramCallScenarioData(
            userId = args.userId,
            callId = args.callId,
            recipient = args.recipient),
        ToProtoContext()
    )
}

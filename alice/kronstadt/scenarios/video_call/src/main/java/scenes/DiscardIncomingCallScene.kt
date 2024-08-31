package ru.yandex.alice.kronstadt.scenarios.video_call.scenes

import org.apache.logging.log4j.LogManager
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.RelevantResponse
import ru.yandex.alice.kronstadt.core.RunOnlyResponse
import ru.yandex.alice.kronstadt.core.VideoCallCapability
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfo
import ru.yandex.alice.kronstadt.core.layout.Layout
import ru.yandex.alice.kronstadt.core.scenario.AbstractNoargScene
import ru.yandex.alice.kronstadt.scenarios.video_call.DECLINE_CALL_INTENT
import ru.yandex.alice.kronstadt.scenarios.video_call.analytics.DiscardVideoCallAction
import ru.yandex.alice.kronstadt.scenarios.video_call.domain.DiscardVideoCallDirective
import ru.yandex.alice.kronstadt.scenarios.video_call.utils.VideoCallCapabilityExtension.getIncomingTelegramCallData

@Component
object DiscardIncomingCallScene
    : AbstractNoargScene<Any>(
    name = "discard_incoming_call_scene"
) {
    private val logger = LogManager.getLogger(DiscardIncomingCallScene::class.java)

    override fun render(request: MegaMindRequest<Any>): RelevantResponse<Any> {
        val incomingCallData = request.videoCallCapability!!.getIncomingTelegramCallData()
                ?: error("Incoming telegram call data not found in videoCall state")
        return RunOnlyResponse(
            layout = Layout.directiveOnlyLayout(
                directives = with(incomingCallData.callOwnerData) {
                    listOf(
                        getDiscardVideoCallDirective(callId, userId))
                }),
                state = null,
                analyticsInfo = AnalyticsInfo(
                    intent = DECLINE_CALL_INTENT,
                    actions = listOf(DiscardVideoCallAction),
                ),
                isExpectsRequest = false
        )
    }
}

fun getDiscardVideoCallDirective(
    callId: String, userId: String
) = DiscardVideoCallDirective(
    providerData = DiscardVideoCallDirective.TelegramDiscardVideoCallData(
        callOwnerData = VideoCallCapability.TelegramVideoCallOwnerData(
            callId = callId,
            userId = userId
        )
    ))
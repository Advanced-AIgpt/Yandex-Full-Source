package ru.yandex.alice.kronstadt.scenarios.video_call.scenes

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.RelevantResponse
import ru.yandex.alice.kronstadt.core.RunOnlyResponse
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfo
import ru.yandex.alice.kronstadt.core.layout.Layout
import ru.yandex.alice.kronstadt.core.scenario.AbstractNoargScene
import ru.yandex.alice.kronstadt.scenarios.video_call.HANGUP_CALL_INTENT
import ru.yandex.alice.kronstadt.scenarios.video_call.analytics.HangupVideoCallAction
import ru.yandex.alice.kronstadt.scenarios.video_call.utils.VideoCallCapabilityExtension.getActiveCall
import ru.yandex.alice.kronstadt.scenarios.video_call.utils.VideoCallCapabilityExtension.getOutgoingCall

@Component
object HangupCallScene
    : AbstractNoargScene<Any>(
    name = "hangup_call_scene"
) {

    override fun render(request: MegaMindRequest<Any>): RelevantResponse<Any> {
        val currentCallData = request.videoCallCapability!!.getActiveCall()
            ?: request.videoCallCapability!!.getOutgoingCall()
            ?: error("Active or outgoing telegram call data not found in videoCall state")
        return RunOnlyResponse(
            layout = Layout.directiveOnlyLayout(
                directives = with(currentCallData.callOwnerData) {
                    listOf(
                        getDiscardVideoCallDirective(callId, userId))
                }),
            state = null,
            analyticsInfo = AnalyticsInfo(
                intent = HANGUP_CALL_INTENT,
                actions = listOf(HangupVideoCallAction)
            ),
            isExpectsRequest = false
        )
    }
}
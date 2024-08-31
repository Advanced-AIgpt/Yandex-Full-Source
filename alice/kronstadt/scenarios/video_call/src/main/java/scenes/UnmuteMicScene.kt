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
import ru.yandex.alice.kronstadt.scenarios.video_call.VIDEO_CALL_UNMUTE_MIC
import ru.yandex.alice.kronstadt.scenarios.video_call.analytics.UnmuteMicAction
import ru.yandex.alice.kronstadt.scenarios.video_call.domain.UnmuteMicDirective
import ru.yandex.alice.kronstadt.scenarios.video_call.utils.VideoCallCapabilityExtension.getActiveOrOutgoingCall

@Component
object VideoCallUnmuteMicScene
    : AbstractNoargScene<Any>(
    name = "unmute_mic_scene"
) {
    private val logger = LogManager.getLogger(VideoCallUnmuteMicScene::class.java)

    override fun render(request: MegaMindRequest<Any>): RelevantResponse<Any> {
        val callData = request.videoCallCapability!!.getActiveOrOutgoingCall()
            ?: error("Active telegram call data not found in videoCall state")
        return RunOnlyResponse(
            layout = Layout.directiveOnlyLayout(
                directives = listOf(getUnmuteMicDirective(callData.callOwnerData))),
            state = null,
            analyticsInfo = AnalyticsInfo(
                intent = VIDEO_CALL_UNMUTE_MIC,
                actions = listOf(UnmuteMicAction),
            ),
            isExpectsRequest = false
        )
    }
}

private fun getUnmuteMicDirective(
    callOwnerData: VideoCallCapability.TelegramVideoCallOwnerData
) = UnmuteMicDirective(
    providerData = UnmuteMicDirective.TelegramUnmuteMicData(
        callOwnerData = VideoCallCapability.TelegramVideoCallOwnerData(
            callId = callOwnerData.callId,
            userId = callOwnerData.userId
        )
    ))
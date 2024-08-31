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
import ru.yandex.alice.kronstadt.scenarios.video_call.VIDEO_CALL_MUTE_MIC
import ru.yandex.alice.kronstadt.scenarios.video_call.analytics.MuteMicAction
import ru.yandex.alice.kronstadt.scenarios.video_call.domain.MuteMicDirective
import ru.yandex.alice.kronstadt.scenarios.video_call.utils.VideoCallCapabilityExtension.getActiveOrOutgoingCall

@Component
object VideoCallMuteMicScene
    : AbstractNoargScene<Any>(
    name = "mute_mic_scene"
) {
    private val logger = LogManager.getLogger(VideoCallMuteMicScene::class.java)

    override fun render(request: MegaMindRequest<Any>): RelevantResponse<Any> {
        val callData = request.videoCallCapability!!.getActiveOrOutgoingCall()
            ?: error("Active telegram call data not found in videoCall state")
        return RunOnlyResponse(
            layout = Layout.directiveOnlyLayout(
                directives = listOf(getMuteMicDirective(callData.callOwnerData))),
            state = null,
            analyticsInfo = AnalyticsInfo(
                intent = VIDEO_CALL_MUTE_MIC,
                actions = listOf(MuteMicAction),
            ),
            isExpectsRequest = false
        )
    }
}

private fun getMuteMicDirective(
    callOwnerData: VideoCallCapability.TelegramVideoCallOwnerData
) = MuteMicDirective(
    providerData = MuteMicDirective.TelegramMuteMicData(
        callOwnerData = VideoCallCapability.TelegramVideoCallOwnerData(
            callId = callOwnerData.callId,
            userId = callOwnerData.userId
        )
    ))

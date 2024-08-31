package ru.yandex.alice.kronstadt.scenarios.video_call.scenes

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.RelevantResponse
import ru.yandex.alice.kronstadt.core.RunOnlyResponse
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfo
import ru.yandex.alice.kronstadt.core.layout.Layout
import ru.yandex.alice.kronstadt.core.scenario.AbstractNoargScene
import ru.yandex.alice.kronstadt.scenarios.video_call.STATE_UPDATES_INTENT

@Component
object VideoCallStateUpdateScene
    : AbstractNoargScene<Any>(name = "video_call_state_update_scene"
) {

    override fun render(request: MegaMindRequest<Any>): RelevantResponse<Any> {
        return RunOnlyResponse(
            layout = Layout.silence(),
            state = null,
            analyticsInfo = AnalyticsInfo(intent = STATE_UPDATES_INTENT),
            isExpectsRequest = false
        )
    }
}
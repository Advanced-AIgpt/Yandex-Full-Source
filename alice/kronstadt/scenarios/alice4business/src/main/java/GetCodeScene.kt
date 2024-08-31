package ru.yandex.alice.kronstadt.scenarios.alice4business

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.RelevantResponse
import ru.yandex.alice.kronstadt.core.RunOnlyResponse
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfo
import ru.yandex.alice.kronstadt.core.directive.MordoviaCommandDirective
import ru.yandex.alice.kronstadt.core.layout.Layout
import ru.yandex.alice.kronstadt.core.scenario.AbstractScene

@Component
object GetCodeScene : AbstractScene<Any, DeviceLockState>("GET_CODE_SCENE", DeviceLockState::class) {
    override fun render(request: MegaMindRequest<Any>, deviceLockState: DeviceLockState): RelevantResponse<Any> {
        val activationCode = deviceLockState.code ?: ACTIVATION_CODE_FALLBACK

        return RunOnlyResponse(
            layout = Layout(
                shouldListen = false,
                outputSpeech = null,
                directives = listOf(
                    MordoviaCommandDirective(
                        command = WEBVIEW_COMMAND_SET_CODE,
                        meta = mapOf("code" to activationCode),
                        viewKey = WEBVIEW_VIEW_KEY
                    )
                ),
            ),
            state = null,
            analyticsInfo = AnalyticsInfo("alice4business.device_lock.code_update"),
            isExpectsRequest = true
        )
    }
}

package ru.yandex.alice.kronstadt.scenarios.alice4business

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.RelevantResponse
import ru.yandex.alice.kronstadt.core.RunOnlyResponse
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfo
import ru.yandex.alice.kronstadt.core.layout.Layout
import ru.yandex.alice.kronstadt.core.scenario.AbstractScene
import ru.yandex.alice.kronstadt.core.text.Phrases

internal const val ALICE4BUSINESS_DEVICE_LOCK_FAILURE = "alice4business.device_lock.failure"

@Component
open class LockFailureScene(val phrases: Phrases) :
    AbstractScene<Any, Boolean>(ALICE4BUSINESS_DEVICE_LOCK_FAILURE, Boolean::class) {

    override fun render(request: MegaMindRequest<Any>, silent: Boolean): RelevantResponse<Any> {
        val layout = Layout.builder().shouldListen(!silent)
        if (!silent) {
            layout.outputSpeech(phrases.getRandom("alice4business.device_lock.failure", request.random))
        }
        return RunOnlyResponse(
            layout = layout.build(),
            state = null,
            analyticsInfo = AnalyticsInfo(ALICE4BUSINESS_DEVICE_LOCK_FAILURE),
            isExpectsRequest = true // ???
        )
    }
}

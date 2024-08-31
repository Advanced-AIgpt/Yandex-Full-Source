package ru.yandex.alice.kronstadt.scenarios.alice4business

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.RelevantResponse
import ru.yandex.alice.kronstadt.core.RunOnlyResponse
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfo
import ru.yandex.alice.kronstadt.core.directive.GoHomeDirective
import ru.yandex.alice.kronstadt.core.layout.Layout
import ru.yandex.alice.kronstadt.core.scenario.AbstractNoargScene
import ru.yandex.alice.kronstadt.core.text.Phrases

internal const val ALICE4BUSINESS_DEVICE_LOCK_UNLOCKED = "alice4business.device_lock.unlocked"

@Component
open class StationUnlockedScene(val phrases: Phrases) : AbstractNoargScene<Any>("DEVICE_UNLOCKED") {
    override fun render(request: MegaMindRequest<Any>): RelevantResponse<Any> {
        return RunOnlyResponse(
            layout = Layout(
                outputSpeech = phrases["alice4business.device_lock.device_unlocked"],
                // to hide activation screen
                directives = listOf(GoHomeDirective),
                shouldListen = false,
            ),
            state = null,
            analyticsInfo = AnalyticsInfo(ALICE4BUSINESS_DEVICE_LOCK_UNLOCKED),
            isExpectsRequest = false
        )
    }
}

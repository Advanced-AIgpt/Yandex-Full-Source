package ru.yandex.alice.kronstadt.core.convert.response

import ru.yandex.alice.kronstadt.core.ScenarioMeta
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfo
import ru.yandex.alice.megamind.protos.scenarios.AnalyticsInfo.TAnalyticsInfo

open class AnalyticsInfoConverter(private val scenarioMeta: ScenarioMeta) {
    fun convert(src: AnalyticsInfo): TAnalyticsInfo {
        val scenario = scenarioMeta

        val builder = TAnalyticsInfo.newBuilder()
            .setIntent(src.intent)
            .setProductScenarioName(scenario.productScenarioName)

        src.actions.forEach { action -> builder.addActions(action.toProto()) }
        src.objects.forEach { obj -> builder.addObjects(obj.toProto()) }
        src.events.forEach { event -> builder.addEvents(event.toProto()) }

        return builder.build()
    }
}

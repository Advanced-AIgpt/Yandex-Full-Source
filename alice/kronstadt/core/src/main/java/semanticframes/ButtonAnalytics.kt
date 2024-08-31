package ru.yandex.alice.kronstadt.core.semanticframes

import ru.yandex.alice.kronstadt.core.ScenarioMeta
import ru.yandex.alice.megamind.protos.common.Atm.TAnalyticsTrackingModule

data class ButtonAnalytics(
    val purpose: String,
    val originInfo: String = ""
) {

    fun toProto(currentScenario: ScenarioMeta): TAnalyticsTrackingModule {
        return TAnalyticsTrackingModule.newBuilder()
            .setProductScenario(currentScenario.productScenarioName)
            .setOrigin(TAnalyticsTrackingModule.EOrigin.Scenario)
            .setPurpose(purpose)
            .setOriginInfo(originInfo)
            .build()
    }
}

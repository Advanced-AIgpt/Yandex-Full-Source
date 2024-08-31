package ru.yandex.alice.kronstadt.scenarios.video_call.domain

import ru.yandex.alice.kronstadt.core.domain.scenariodata.ScenarioData

data class OutgoingTelegramCallScenarioData(
    val userId: String,
    val recipient: ProviderContactData,
) : ScenarioData

package ru.yandex.alice.kronstadt.scenarios.video_call.domain

import ru.yandex.alice.kronstadt.core.domain.scenariodata.ScenarioData

data class CurrentTelegramCallScenarioData(
    val userId: String,
    val callId: String,
    val recipient: ProviderContactData,
) : ScenarioData
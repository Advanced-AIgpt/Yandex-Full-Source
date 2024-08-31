package ru.yandex.alice.kronstadt.scenarios.video_call.domain

import ru.yandex.alice.kronstadt.core.domain.scenariodata.ScenarioData

data class IncomingTelegramCallScenarioData(
    val userId: String,
    val callId: String,
    val caller: ProviderContactData,
) : ScenarioData

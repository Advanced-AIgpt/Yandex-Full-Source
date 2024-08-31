package ru.yandex.alice.kronstadt.core

import ru.yandex.alice.protos.data.scenario.Data.TScenarioData

data class DivRenderData(
    val cardId: String,
    val scenarioData: TScenarioData
)
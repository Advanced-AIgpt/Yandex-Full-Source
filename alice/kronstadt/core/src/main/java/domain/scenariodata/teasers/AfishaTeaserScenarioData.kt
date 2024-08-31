package ru.yandex.alice.kronstadt.core.domain.scenariodata.teasers

import ru.yandex.alice.kronstadt.core.domain.scenariodata.ScenarioData

data class AfishaTeaserScenarioData(
    val title: String?,
    val imageUrl: String?,
    val date: String?,
    val place: String?,
    val contentRating: String?
) : ScenarioData

package ru.yandex.alice.kronstadt.core.domain.scenariodata.teasers

import ru.yandex.alice.kronstadt.core.domain.scenariodata.ScenarioData

data class TeasersPreviewData(
    val teaserPreviews: List<TeaserPreview>
) : ScenarioData {
    data class TeaserPreview(
        val teaserConfigData: TeaserConfigData,
        val teaserName: String,
        val previewScenarioData: ScenarioData
    )
}

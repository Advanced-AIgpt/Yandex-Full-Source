package ru.yandex.alice.kronstadt.core.domain.scenariodata.teasers

import ru.yandex.alice.kronstadt.core.domain.scenariodata.ScenarioData

data class TeaserSkillCardData(
    val skillInfo: SkillInfoData,
    val text: String?,
    val title: String?,
    val imageUrl: String?,
    val tapAction: TapAction?,
    val teaserId: String
) : ScenarioData {
    data class TapAction(
        val payload: String?,
        val activationCommand: String?
    )

    data class SkillInfoData(
        val name: String,
        val logo: String?,
        val skillId: String
    )
}

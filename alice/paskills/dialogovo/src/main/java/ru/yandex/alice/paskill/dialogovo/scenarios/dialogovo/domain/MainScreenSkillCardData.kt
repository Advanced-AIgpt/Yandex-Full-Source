package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.domain

import ru.yandex.alice.kronstadt.core.domain.scenariodata.ScenarioData

data class MainScreenSkillCardData(
    val skillInfoData: SkillInfoData,
    val skillResponse: SkillResponse? = null
) : ScenarioData {
    data class SkillResponse(
        val title: String?,
        val text: String?,
        val imageUrl: String?,
        val buttons: List<Button> = emptyList(),
        val tapAction: TapAction?
    )

    data class Button(
        val title: String,
        val payload: String?,
    )

    data class TapAction(
        val payload: String?,
        val activationCommand: String?
    )
}

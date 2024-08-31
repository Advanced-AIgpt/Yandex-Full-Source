package ru.yandex.alice.paskill.dialogovo.domain.show

import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo
import ru.yandex.alice.paskill.dialogovo.external.v1.response.show.ShowType

data class ShowInfo(
    val id: String,
    val name: String,
    val nameTts: String? = null,
    val description: String,
    val skillInfo: SkillInfo,
    val showType: ShowType = ShowType.MORNING,
    val onAir: Boolean = true,
    val personalizationEnabled: Boolean = false
)

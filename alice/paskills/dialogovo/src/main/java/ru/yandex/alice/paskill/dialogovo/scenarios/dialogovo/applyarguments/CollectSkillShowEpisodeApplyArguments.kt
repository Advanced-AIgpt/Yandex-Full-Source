package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.applyarguments

import ru.yandex.alice.kronstadt.core.applyarguments.ApplyArguments
import ru.yandex.alice.paskill.dialogovo.external.v1.response.show.ShowType

data class CollectSkillShowEpisodeApplyArguments(
    val skillId: String,
    val showType: ShowType
) : ApplyArguments

package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics

import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfoObject
import ru.yandex.alice.megamind.protos.analytics.scenarios.dialogovo.Dialogovo
import ru.yandex.alice.megamind.protos.scenarios.AnalyticsInfo

data class SuggestedSkillExitInfoObject constructor(
    val skillId: String,
    val containsWeakDeactivateForm: Boolean,
    val doNotUnderstandCounter: Long,
) : AnalyticsInfoObject(skillId, "external_skill.suggested_skill_exit", "Предложение выйти из навыка") {

    override fun fillProtoField(protoBuilder: AnalyticsInfo.TAnalyticsInfo.TObject.Builder):
        AnalyticsInfo.TAnalyticsInfo.TObject.Builder =
        protoBuilder.setSkillProactivitySuggestExit(
            Dialogovo.TSkillSuggestedExit.newBuilder().setReason(
                if (containsWeakDeactivateForm) "User request contains weak deactivate form"
                else "Do not understand counter reached it's limit: $doNotUnderstandCounter"
            )
        )
}

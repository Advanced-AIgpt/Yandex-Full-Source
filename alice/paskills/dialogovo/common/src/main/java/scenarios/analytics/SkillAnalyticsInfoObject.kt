package ru.yandex.alice.paskill.dialogovo.scenarios.analytics

import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfoObject
import ru.yandex.alice.megamind.protos.analytics.scenarios.dialogovo.Dialogovo.TSkill
import ru.yandex.alice.megamind.protos.scenarios.AnalyticsInfo.TAnalyticsInfo.TObject
import ru.yandex.alice.paskill.dialogovo.domain.DeveloperType
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo
import javax.annotation.Nonnull

data class SkillAnalyticsInfoObject(
    val skillInfo: SkillInfo
) : AnalyticsInfoObject(skillInfo.id, skillInfo.name, String.format("Навык «%s»", skillInfo.name)) {
    @Nonnull
    public override fun fillProtoField(@Nonnull protoBuilder: TObject.Builder): TObject.Builder {
        val payload = TSkill.newBuilder()
            .setId(skillInfo.id)
            .setName(skillInfo.name)
            .setDeveloperType(DeveloperType.getProtoDeveloperType(skillInfo.developerType))
            .setDeveloperName(skillInfo.developerName)
            .setCategory(skillInfo.category)
            .setVoice(skillInfo.voice.code)
            .setAdBlockId(skillInfo.getAdBlockId().orElse(""))
            .setBackendType(
                if (!skillInfo.functionId.isNullOrEmpty())
                    TSkill.EBackendType.CLOUD_FUNCTION
                else TSkill.EBackendType.WEBHOOK
            )
        return protoBuilder.setSkill(payload)
    }
}

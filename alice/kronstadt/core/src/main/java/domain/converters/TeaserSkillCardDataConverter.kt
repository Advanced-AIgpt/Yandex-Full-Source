package ru.yandex.alice.kronstadt.core.domain.converters

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoContext
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoConverter
import ru.yandex.alice.kronstadt.core.domain.scenariodata.teasers.TeaserSkillCardData
import ru.yandex.alice.protos.data.scenario.dialogovo.Skill.TDialogovoSkillTeaserData
import ru.yandex.alice.protos.data.scenario.dialogovo.Skill.TDialogovoSkillTeaserData.TSkillInfo

@Component
class TeaserSkillCardDataConverter : ToProtoConverter<TeaserSkillCardData, TDialogovoSkillTeaserData> {
    override fun convert(src: TeaserSkillCardData, ctx: ToProtoContext): TDialogovoSkillTeaserData {
        return TDialogovoSkillTeaserData.newBuilder().apply {
            skillInfo = convertSkillInfo(src)
            action = convertAction(src.teaserId)
            src.text?.let { text = it }
            src.title?.let { title = it }
            src.imageUrl.let { imageUrl = it }
        }.build()
    }

    private fun convertSkillInfo(src: TeaserSkillCardData): TSkillInfo {
        val skillInfo = TSkillInfo.newBuilder().setSkillId(src.skillInfo.skillId).setName(src.skillInfo.name)
        src.skillInfo.logo?.let { skillInfo.logo = it }
        return skillInfo.build()
    }

    private fun convertAction(id: String) = "@@mm_deeplink#$id"
}

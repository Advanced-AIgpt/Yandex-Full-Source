package ru.yandex.alice.paskill.dialogovo.scenarios

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoContext
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoConverter
import ru.yandex.alice.kronstadt.core.domain.converters.TeaserSkillCardDataConverter
import ru.yandex.alice.kronstadt.core.domain.scenariodata.ScenarioData
import ru.yandex.alice.kronstadt.core.domain.scenariodata.teasers.TeaserSkillCardData
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.converters.DialogovoSkillCardDataConverter
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.domain.DialogovoSkillCardData
import ru.yandex.alice.protos.data.scenario.Data.TScenarioData

@Component
open class DialogovoScenarioDataConverter(
    private val dialogovoSkillCardDataConverter: DialogovoSkillCardDataConverter,
    private val teaserSkillCardDataConverter: TeaserSkillCardDataConverter
) : ToProtoConverter<ScenarioData, TScenarioData> {
    override fun convert(src: ScenarioData, ctx: ToProtoContext): TScenarioData {
        when (src) {
            is DialogovoSkillCardData -> return convertDialogovoSkillCardScenarioData(src, ctx)
            is TeaserSkillCardData -> return convertTeaserSkillCardData(src, ctx)
        }
        throw RuntimeException("Can't find converter for scenario_data of type: " + src.javaClass.name)
    }

    private fun convertDialogovoSkillCardScenarioData(
        src: DialogovoSkillCardData,
        ctx: ToProtoContext
    ): TScenarioData {
        return TScenarioData.newBuilder()
            .setDialogovoSkillCardData(dialogovoSkillCardDataConverter.convert(src, ctx))
            .build()
    }

    private fun convertTeaserSkillCardData(
        src: TeaserSkillCardData,
        ctx: ToProtoContext
    ): TScenarioData {
        return TScenarioData.newBuilder().setDialogovoTeaserCardData(
            teaserSkillCardDataConverter.convert(src, ctx)
        ).build()
    }
}

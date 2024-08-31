package ru.yandex.alice.kronstadt.core.domain.converters

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoContext
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoConverter
import ru.yandex.alice.kronstadt.core.domain.scenariodata.teasers.AfishaTeaserScenarioData
import ru.yandex.alice.kronstadt.core.domain.scenariodata.ScenarioData
import ru.yandex.alice.kronstadt.core.domain.scenariodata.teasers.ScreenSaverScenarioData
import ru.yandex.alice.kronstadt.core.domain.scenariodata.teasers.TeaserConfigData
import ru.yandex.alice.kronstadt.core.domain.scenariodata.teasers.TeaserSkillCardData
import ru.yandex.alice.kronstadt.core.domain.scenariodata.teasers.TeasersPreviewData
import ru.yandex.alice.protos.data.scenario.centaur.teasers.TeaserSettings
import ru.yandex.alice.protos.data.scenario.centaur.teasers.TeaserSettings.TCentaurTeaserConfigData

@Component
class TeaserPreviewDataConverter(
    private val afishaConverter: AfishaConverter,
    private val screenSaverConverter: ScreenSaverConverter,
    private val teaserSkillCardDataConverter: TeaserSkillCardDataConverter,
) : ToProtoConverter<TeasersPreviewData, TeaserSettings.TTeasersPreviewData> {

    override fun convert(src: TeasersPreviewData, ctx: ToProtoContext): TeaserSettings.TTeasersPreviewData {
        val teasersPreviewData = TeaserSettings.TTeasersPreviewData.newBuilder()
        for (teaserPreview in src.teaserPreviews) {
            teasersPreviewData.addTeaserPreviews(convertTeaserPreview(teaserPreview))
        }
        return teasersPreviewData.build()
    }

    private fun convertTeaserPreview(src: TeasersPreviewData.TeaserPreview):
        TeaserSettings.TTeasersPreviewData.TTeaserPreview {
        return TeaserSettings.TTeasersPreviewData.TTeaserPreview.newBuilder()
            .setTeaserName(src.teaserName)
            .setTeaserConfigData(convertTeaserConfig(src.teaserConfigData))
            .setTeaserPreviewScenarioData(convertPreviewScenarioData(src.previewScenarioData))
            .build()
    }

    private fun convertTeaserConfig(src: TeaserConfigData) =
        TCentaurTeaserConfigData.newBuilder().setTeaserType(src.teaserType).setTeaserId(src.teaserId).build()

    private fun convertPreviewScenarioData(src: ScenarioData): TeaserSettings.TTeaserPreviewScenarioData {
        val data = TeaserSettings.TTeaserPreviewScenarioData.newBuilder()
        when (src) {
            is AfishaTeaserScenarioData -> data.afishaTeaserData = afishaConverter.convert(src, ToProtoContext())
            is ScreenSaverScenarioData -> data.screenSaverData = screenSaverConverter.convert(src, ToProtoContext())
            is TeaserSkillCardData -> data.dialogovoTeaserCardData =
                teaserSkillCardDataConverter.convert(src, ToProtoContext())
        }
        return data.build()
    }
}

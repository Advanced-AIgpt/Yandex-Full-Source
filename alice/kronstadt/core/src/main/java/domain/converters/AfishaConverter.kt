package ru.yandex.alice.kronstadt.core.domain.converters

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoContext
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoConverter
import ru.yandex.alice.kronstadt.core.domain.scenariodata.teasers.AfishaTeaserScenarioData
import ru.yandex.alice.protos.data.scenario.afisha.Afisha.TAfishaTeaserData

@Component
class AfishaConverter : ToProtoConverter<AfishaTeaserScenarioData, TAfishaTeaserData> {
    override fun convert(src: AfishaTeaserScenarioData, ctx: ToProtoContext): TAfishaTeaserData {
        return TAfishaTeaserData.newBuilder().apply {
            src.title?.let { title = it }
            src.imageUrl?.let { imageUrl = it }
            src.date?.let { date = it }
            src.place?.let { place = it }
            src.contentRating?.let { contentRating = it }
        }.build()
    }
}

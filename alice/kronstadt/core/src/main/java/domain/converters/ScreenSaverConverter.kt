package ru.yandex.alice.kronstadt.core.domain.converters

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoContext
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoConverter
import ru.yandex.alice.kronstadt.core.domain.scenariodata.teasers.ScreenSaverScenarioData
import ru.yandex.alice.protos.data.scenario.photoframe.ScreenSaver.TScreenSaverData

@Component
class ScreenSaverConverter : ToProtoConverter<ScreenSaverScenarioData, TScreenSaverData> {
    override fun convert(src: ScreenSaverScenarioData, ctx: ToProtoContext): TScreenSaverData {
        return TScreenSaverData.newBuilder().setImageUrl(src.imageUrl).build()
    }
}

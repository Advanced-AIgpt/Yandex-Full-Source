package ru.yandex.alice.kronstadt.core.directive

import com.yandex.div.dsl.context.CardWithTemplates
import ru.yandex.alice.kronstadt.core.domain.scenariodata.teasers.TeaserConfigData
import java.time.Duration

data class AddGalleryCardDirective(
    val cardId: String,
    val teaserConfigData: TeaserConfigData,
    val showTime: Duration = Duration.ofSeconds(30),
    val div2Card: CardWithTemplates? = null,
) : MegaMindDirective

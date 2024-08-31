package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.analytics

import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfoEvent
import ru.yandex.alice.megamind.protos.analytics.scenarios.dialogovo.Dialogovo.TRecipeProactiveSuggestEvent
import ru.yandex.alice.megamind.protos.analytics.scenarios.dialogovo.Dialogovo.TRecipeProactiveSuggestEvent.TSuggestType
import ru.yandex.alice.megamind.protos.scenarios.AnalyticsInfo.TAnalyticsInfo.TEvent
import javax.annotation.Nonnull

object MusicSuggestedEvent : AnalyticsInfoEvent() {
    override fun fillProtoField(@Nonnull protoBuilder: TEvent.Builder): TEvent.Builder {
        val protoEvent = TRecipeProactiveSuggestEvent.newBuilder()
            .setSuggestType(TSuggestType.Music)
            .build()
        return protoBuilder.setRecipeProactiveSuggest(protoEvent)
    }
}

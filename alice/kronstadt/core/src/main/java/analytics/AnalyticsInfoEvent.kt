package ru.yandex.alice.kronstadt.core.analytics

import ru.yandex.alice.megamind.protos.scenarios.AnalyticsInfo.TAnalyticsInfo.TEvent
import java.time.Instant

/**
 * Marker interface for analytics info events
 */
abstract class AnalyticsInfoEvent {

    fun toProto(): TEvent {
        val protoBuilder: TEvent.Builder = TEvent.newBuilder()
            .setTimestamp(Instant.now().toEpochMilli())

        return fillProtoField(protoBuilder).build()
    }

    protected abstract fun fillProtoField(protoBuilder: TEvent.Builder): TEvent.Builder
}

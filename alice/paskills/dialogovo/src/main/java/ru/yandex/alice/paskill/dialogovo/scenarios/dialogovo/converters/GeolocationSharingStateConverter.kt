package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.converters

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoContext
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoConverter
import ru.yandex.alice.paskill.dialogovo.proto.DialogovoStateProto.State.TGeolocationSharingState
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState.GeolocationSharingState

@Component
open class GeolocationSharingStateConverter : ToProtoConverter<GeolocationSharingState, TGeolocationSharingState> {
    override fun convert(src: GeolocationSharingState, ctx: ToProtoContext): TGeolocationSharingState {
        val builder = TGeolocationSharingState.newBuilder()
            .setIsRequested(src.isRequested)
            .setAllowedSharingUntilTime(src.allowedSharingUntilTime?.toEpochMilli() ?: 0L)
        return builder.build()
    }
}

package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.converters

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoContext
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoConverter
import ru.yandex.alice.paskill.dialogovo.proto.DialogovoStateProto.State.TProductActivationState
import ru.yandex.alice.paskill.dialogovo.proto.DialogovoStateProto.State.TProductActivationState.TActivationType
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState.ProductActivationState

@Component
open class ProductActivationStateConverter : ToProtoConverter<ProductActivationState, TProductActivationState> {
    override fun convert(src: ProductActivationState, ctx: ToProtoContext): TProductActivationState {
        return TProductActivationState.newBuilder()
            .setMusicAttemptCount(src.musicAttemptCount ?: 0)
            .setActivationType(convertActivationType(src.activationType))
            .build()
    }

    private fun convertActivationType(activationType: DialogovoState.ActivationType): TActivationType {
        return when (activationType) {
            DialogovoState.ActivationType.MUSIC -> TActivationType.MUSIC
        }
    }
}

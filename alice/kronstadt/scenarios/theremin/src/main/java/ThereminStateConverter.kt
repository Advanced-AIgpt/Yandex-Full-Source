package ru.yandex.alice.paskill.dialogovo.scenarios.theremin

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.convert.StateConverter
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoContext
import ru.yandex.alice.megamind.protos.scenarios.RequestProto
import ru.yandex.alice.paskill.dialogovo.proto.DialogovoStateProto
import ru.yandex.alice.paskill.dialogovo.proto.DialogovoStateProto.State.ThereminModeState

@Component
open class ThereminStateConverter : StateConverter<ThereminState> {

    override fun convert(src: ThereminState, ctx: ToProtoContext): DialogovoStateProto.State {
        return DialogovoStateProto.State.newBuilder().apply {
            thereminState = ThereminModeState.newBuilder()
                .setIsInternal(src.isInternal)
                .setModeId(src.modeId)
                .build()
        }.build()
    }

    override fun convert(src: RequestProto.TScenarioBaseRequest): ThereminState? {
        // TODO: use separate state TTL and IsNewSession handler for each state
        return if (src.hasState() &&
            src.state.serializedSize > 0 &&
            src.state.`is`(DialogovoStateProto.State::class.java)) {
            val unpacked = src.state.unpack(DialogovoStateProto.State::class.java)

            return if (unpacked.hasThereminState()) {
                ThereminState(unpacked.thereminState.isInternal, unpacked.thereminState.modeId)
            } else null
        } else {
            null
        }
    }


}

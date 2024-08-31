package ru.yandex.alice.kronstadt.core.convert.response

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.ActionSpace
import ru.yandex.alice.megamind.protos.scenarios.ActionSpaceProto

@Component
open class ActionSpaceConverter(
   private val semanticFrameRequestDataConverter: SemanticFrameRequestDataConverter
) : ToProtoConverter<ActionSpace, ActionSpaceProto.TActionSpace> {
    override fun convert(src: ActionSpace, ctx: ToProtoContext): ActionSpaceProto.TActionSpace {
        val builder = ActionSpaceProto.TActionSpace.newBuilder()
        builder.addAllNluHints(
            src.nluHints.map {
                ActionSpaceProto.TActionSpace.TNluHint
                    .newBuilder()
                    .setActionId(it.actionId)
                    .setSemanticFrameName(it.semanticFrameName)
                    .build()
            }
        )
        for ((key, value) in src.effects) {
            if (value is ActionSpace.TypedSemanticFrameEffect) {
                val semanticFrameRequestData = semanticFrameRequestDataConverter.convert(value.semanticFrame, ctx)
                builder.putActions(
                    key,
                    ActionSpaceProto.TActionSpace.TAction
                        .newBuilder()
                        .setSemanticFrame(semanticFrameRequestData)
                        .build()
                )
            }
        }
        return builder.build()
    }
}

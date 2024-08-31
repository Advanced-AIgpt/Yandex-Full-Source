package ru.yandex.alice.kronstadt.core.convert.response

import com.google.protobuf.Any
import com.google.protobuf.BoolValue
import ru.yandex.alice.kronstadt.core.GrpcResponseBody
import ru.yandex.alice.kronstadt.core.IScenarioResponseBody
import ru.yandex.alice.kronstadt.core.ScenarioMeta
import ru.yandex.alice.kronstadt.core.ScenarioResponseBody
import ru.yandex.alice.kronstadt.core.StackEngine
import ru.yandex.alice.kronstadt.core.StackEngineAction
import ru.yandex.alice.kronstadt.core.StackEngineEffect
import ru.yandex.alice.kronstadt.core.convert.StateConverter
import ru.yandex.alice.megamind.protos.common.EffectOptions.TEffectOptions
import ru.yandex.alice.megamind.protos.scenarios.ResponseProto.TScenarioResponseBody
import ru.yandex.alice.megamind.protos.scenarios.StackEngineProto.TStackEngine
import ru.yandex.alice.megamind.protos.scenarios.StackEngineProto.TStackEngineAction
import ru.yandex.alice.megamind.protos.scenarios.StackEngineProto.TStackEngineAction.TNewSession
import ru.yandex.alice.megamind.protos.scenarios.StackEngineProto.TStackEngineAction.TResetAdd
import ru.yandex.alice.megamind.protos.scenarios.StackEngineProto.TStackEngineEffect

open class ScenarioResponseBodyConverter<State>(
    private val scenarioMeta: ScenarioMeta,
    private val layoutConverter: LayoutConverter,
    private val analyticsInfoConverter: AnalyticsInfoConverter,
    private val stateConverter: StateConverter<State>,
    private val actionConverter: ActionConverter,
    private val actionSpaceConverter: ActionSpaceConverter,
    private val directiveConverter: CallbackDirectiveConverter,
    private val serverDirectiveConverter: ServerDirectiveConverter
) : ToProtoConverter<IScenarioResponseBody<State>, TScenarioResponseBody> {
    override fun convert(src: IScenarioResponseBody<State>, ctx: ToProtoContext): TScenarioResponseBody {
        val builder = TScenarioResponseBody.newBuilder()

        when (src) {
            is ScenarioResponseBody<State> -> {
                builder.layout = layoutConverter.convert(src.layout, ctx)
                builder.expectsRequest = src.isExpectsRequest
                if (src.state != null) {
                    builder.state = Any.pack(stateConverter.convert(src.state, ctx))
                }

                for ((key, value) in ctx.getActions()) {
                    val action = actionConverter.convert(value, ctx)
                    builder.putFrameActions(key, action)
                }
                for ((key, value) in src.actions) {
                    val action = actionConverter.convert(value, ctx)
                    builder.putFrameActions(key, action)
                }
                if (src.stackEngine != null) {
                    builder.stackEngine = convert(src.stackEngine)
                }
                if (src.scenarioData != null) {
                    builder.scenarioData = src.scenarioData
                }
            }
            is GrpcResponseBody<State> -> {
                builder.grpcResponseBuilder.payload = Any.pack(src.grpcResponse)
            }
        }
        builder.analyticsInfo = analyticsInfoConverter.convert(src.analyticsInfo)

        for ((key, value) in src.actionSpaces) {
            val actionSpace = actionSpaceConverter.convert(value, ctx)
            builder.putActionSpaces(key, actionSpace)
        }
        for (directive in src.serverDirectives) {
            val serverDirective = serverDirectiveConverter.convert(directive, ctx)
            builder.addServerDirectives(serverDirective)
        }

        return builder.build()
    }

    private fun convert(stackEngine: StackEngine): TStackEngine {
        return TStackEngine.newBuilder()
            .addAllActions(stackEngine.actions.map { convertAction(it) })
            .build()
    }

    private fun convertAction(action: StackEngineAction): TStackEngineAction {
        return TStackEngineAction.newBuilder().apply {
            when (action) {
                is StackEngineAction.ResetAdd -> this.resetAdd = TResetAdd.newBuilder()
                    .addAllEffects(action.effects.map { effect -> convertEffect(effect) })
                    .build()
                is StackEngineAction.NewSession -> this.newSession = TNewSession.newBuilder().build()

            }
        }.build()
    }

    private fun convertEffect(effect: StackEngineEffect): TStackEngineEffect {
        return TStackEngineEffect.newBuilder().apply {
            when (effect) {
                is StackEngineEffect.CallbackEffect ->
                    setCallback(directiveConverter.convert(effect.directive))
                is StackEngineEffect.ParsedUtteranceEffect ->
                    parsedUtterance = effect.parsedUtterance.toProto(scenarioMeta)
            }
            if (effect.forceShouldListen) {
                options = TEffectOptions.newBuilder()
                    .setForcedShouldListen(BoolValue.of(effect.forceShouldListen))
                    .build()
            }
        }.build()
    }
}

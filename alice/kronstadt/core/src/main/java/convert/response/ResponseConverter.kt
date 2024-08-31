package ru.yandex.alice.kronstadt.core.convert.response

import com.google.protobuf.Any
import ru.yandex.alice.kronstadt.core.ApplyNeededResponse
import ru.yandex.alice.kronstadt.core.BaseRunResponse
import ru.yandex.alice.kronstadt.core.CommitNeededResponse
import ru.yandex.alice.kronstadt.core.IrrelevantResponse
import ru.yandex.alice.kronstadt.core.RunOnlyResponse
import ru.yandex.alice.kronstadt.core.ScenarioResponseBody
import ru.yandex.alice.kronstadt.core.VersionProvider
import ru.yandex.alice.kronstadt.core.applyarguments.ApplyArgumentsConverter
import ru.yandex.alice.megamind.protos.scenarios.ResponseProto.TScenarioResponseBody
import ru.yandex.alice.megamind.protos.scenarios.ResponseProto.TScenarioRunResponse

class ResponseConverter<State>(
    private val applyArgumentsConverter: ApplyArgumentsConverter,
    private val scenarioResponseBodyConverter: ScenarioResponseBodyConverter<State>,
    private val versionProvider: VersionProvider
) {
    @Suppress("UNCHECKED_CAST")
    fun convert(response: BaseRunResponse<State>): TScenarioRunResponse {
        val ctx = ToProtoContext()
        return when (response) {
            is RunOnlyResponse<*> -> convertRunOnly(response as RunOnlyResponse<State>, ctx)
            is CommitNeededResponse<*> -> convertCommitNeeded(
                response as CommitNeededResponse<State>,
                ctx
            )
            is ApplyNeededResponse<*> -> convertApplyNeeded(response as ApplyNeededResponse<State>, ctx)
            is IrrelevantResponse<*> -> convertIrrelevant(response as IrrelevantResponse<State>, ctx)
            else -> throw RuntimeException("Unable to convert response to proto: ${response.javaClass.name}")
        }
    }

    fun convert(response: ScenarioResponseBody<State>): TScenarioResponseBody {
        val ctx = ToProtoContext()
        return scenarioResponseBodyConverter.convert(response, ctx)
    }

    private fun convertRunOnly(src: RunOnlyResponse<State>, ctx: ToProtoContext): TScenarioRunResponse {
        return TScenarioRunResponse.newBuilder()
            .setFeatures(convertFeatures(src))
            .setResponseBody(scenarioResponseBodyConverter.convert(src.body, ctx))
            .setVersion(versionProvider.version)
            .build()
    }

    private fun convertCommitNeeded(
        src: CommitNeededResponse<State>,
        ctx: ToProtoContext
    ): TScenarioRunResponse {
        val applyArgs = Any.pack(applyArgumentsConverter.convert(src.arguments, ctx))

        return TScenarioRunResponse.newBuilder()
            .setFeatures(convertFeatures(src))
            .setCommitCandidate(
                TScenarioRunResponse.TCommitCandidate.newBuilder()
                    .setArguments(applyArgs)
                    .setResponseBody(scenarioResponseBodyConverter.convert(src.body, ctx))
            )
            .setVersion(versionProvider.version)
            .build()
    }

    private fun convertApplyNeeded(src: ApplyNeededResponse<State>, ctx: ToProtoContext): TScenarioRunResponse {
        return TScenarioRunResponse.newBuilder()
            .setFeatures(convertFeatures(src))
            .setApplyArguments(Any.pack(applyArgumentsConverter.convert(src.arguments, ctx)))
            .setVersion(versionProvider.version)
            .build()
    }

    private fun convertIrrelevant(src: IrrelevantResponse<State>, ctx: ToProtoContext): TScenarioRunResponse {
        return TScenarioRunResponse.newBuilder()
            .setFeatures(convertFeatures(src))
            .setResponseBody(scenarioResponseBodyConverter.convert(src.body, ctx))
            .setVersion(versionProvider.version)
            .build()
    }

    private fun convertFeatures(src: BaseRunResponse<State>): TScenarioRunResponse.TFeatures.Builder {
        val features: TScenarioRunResponse.TFeatures.Builder = TScenarioRunResponse.TFeatures.newBuilder()
            .setIsIrrelevant(!src.isRelevant)
        src.features?.playerFeatures?.also { playerFeatures ->
            features.setPlayerFeatures(
                TScenarioRunResponse.TFeatures.TPlayerFeatures
                    .newBuilder()
                    .setRestorePlayer(playerFeatures.restorePlayer)
                    .setSecondsSincePause(playerFeatures.secondsSincePause.toInt())
            )
        }
        return features
    }
}

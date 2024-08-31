package ru.yandex.alice.kronstadt.core

import com.google.protobuf.Message
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfo
import ru.yandex.alice.kronstadt.core.applyarguments.ApplyArguments
import ru.yandex.alice.kronstadt.core.directive.server.ServerDirective
import ru.yandex.alice.kronstadt.core.layout.Layout
import ru.yandex.alice.kronstadt.core.layout.TextWithTts
import ru.yandex.alice.protos.data.scenario.Data.TScenarioData
import java.util.Random

sealed class BaseRunResponse<State>(
    val isRelevant: Boolean,
    val features: Features? = null
)

open class IrrelevantResponse<State> @JvmOverloads constructor(
    val body: ScenarioResponseBody<State>,
    features: Features? = null
) : BaseRunResponse<State>(false, features) {

    constructor(
        layout: Layout,
        state: State?,
        analyticsInfo: AnalyticsInfo,
        isExpectsRequest: Boolean = false,
        actions: Map<String, ActionRef> = mapOf(),
        stackEngine: StackEngine? = null,
        serverDirectives: List<ServerDirective> = listOf(),
        features: Features? = null,
    ) : this(
        ScenarioResponseBody(
            layout = layout,
            state = state,
            analyticsInfo = analyticsInfo,
            isExpectsRequest = isExpectsRequest,
            actions = actions,
            stackEngine = stackEngine,
            serverDirectives = serverDirectives,
        ), features
    )

    interface Factory<State> {
        fun create(request: MegaMindRequest<State>): IrrelevantResponse<State>
    }

    abstract class PhrasesFactory<State> : Factory<State> {

        protected abstract fun getPhrase(random: Random): TextWithTts

        override fun create(request: MegaMindRequest<State>): IrrelevantResponse<State> {
            // подозреваю, что тут бага и нужно использовать `phrasesKey()` с элипсисом
            val textWithTts = getPhrase(request.random)
            val layout = Layout.textWithOutputSpeech(textWithTts, shouldListen = request.voiceSession)
            return IrrelevantResponse(
                layout = layout,
                state = request.state,
                analyticsInfo = AnalyticsInfo.IRRELEVANT,
            )
        }
    }

    companion object {
        const val IRRELEVANT = "irrelevant"
    }
}

sealed class RelevantResponse<State>(features: Features? = null) : BaseRunResponse<State>(true, features)

// RUN
class RunOnlyResponse<State>
@JvmOverloads constructor(
    val body: IScenarioResponseBody<State>,
    features: Features? = null,
) : RelevantResponse<State>(features) {

    constructor(
        layout: Layout,
        state: State?,
        analyticsInfo: AnalyticsInfo,
        isExpectsRequest: Boolean = false,
        actions: Map<String, ActionRef> = mapOf(),
        stackEngine: StackEngine? = null,
        serverDirectives: List<ServerDirective> = listOf(),
        features: Features? = null,
        scenarioData: TScenarioData? = null,
        renderData: List<DivRenderData> = listOf(),
        actionSpaces: Map<String, ActionSpace> = mapOf(),
    ) : this(
        ScenarioResponseBody(
            layout = layout,
            state = state,
            analyticsInfo = analyticsInfo,
            isExpectsRequest = isExpectsRequest,
            actions = actions,
            stackEngine = stackEngine,
            serverDirectives = serverDirectives,
            scenarioData = scenarioData,
            renderData = renderData,
            actionSpaces = actionSpaces,
        ), features
    )

    constructor(
        grpcResponse: Message,
        analyticsInfo: AnalyticsInfo,
        serverDirectives: List<ServerDirective> = listOf(),
        features: Features? = null,
    ) : this(
        GrpcResponseBody(
            grpcResponse = grpcResponse,
            analyticsInfo = analyticsInfo,
            serverDirectives = serverDirectives,
        ), features
    )
}

interface WithArgumentResponse {
    val arguments: ApplyArguments
}

// COMMIT
open class CommitNeededResponse<State>
@JvmOverloads constructor(
    val body: IScenarioResponseBody<State>,
    override val arguments: ApplyArguments,
    features: Features? = null
) : RelevantResponse<State>(features), WithArgumentResponse {

    constructor(
        layout: Layout,
        state: State?,
        analyticsInfo: AnalyticsInfo,
        isExpectsRequest: Boolean,
        arguments: ApplyArguments,
        actions: Map<String, ActionRef> = mapOf(),
        stackEngine: StackEngine? = null,
        serverDirectives: List<ServerDirective> = listOf(),
        features: Features? = null,
    ) : this(
        ScenarioResponseBody(
            layout = layout,
            state = state,
            analyticsInfo = analyticsInfo,
            isExpectsRequest = isExpectsRequest,
            actions = actions,
            stackEngine = stackEngine,
            serverDirectives = serverDirectives,
        ), arguments, features
    )

    constructor(
        grpcResponse: Message,
        analyticsInfo: AnalyticsInfo,
        arguments: ApplyArguments,
        serverDirectives: List<ServerDirective> = listOf(),
        features: Features? = null,
    ) : this(
        GrpcResponseBody(
            grpcResponse = grpcResponse,
            analyticsInfo = analyticsInfo,
            serverDirectives = serverDirectives,
        ), arguments, features
    )
}

enum class CommitResult {
    Success, Error
}

// APPLY
open class ApplyNeededResponse<State>
@JvmOverloads constructor(
    override val arguments: ApplyArguments,
    features: Features? = null
) : RelevantResponse<State>(features), WithArgumentResponse

class RunErrorResponse<State>(val message: String, relevant: Boolean = false) : BaseRunResponse<State>(relevant)

// CONTINUE
open class ContinueNeededResponse<State>
@JvmOverloads constructor(
    override val arguments: ApplyArguments,
    features: Features? = null
) : RelevantResponse<State>(features), WithArgumentResponse

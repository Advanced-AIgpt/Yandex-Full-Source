package ru.yandex.alice.kronstadt.core

import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfo
import ru.yandex.alice.kronstadt.core.directive.server.ServerDirective
import ru.yandex.alice.kronstadt.core.layout.Layout
import ru.yandex.alice.protos.data.scenario.Data.TScenarioData
import java.util.Optional

// see: https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/scenarios/response.proto
data class ScenarioResponseBody<State> @JvmOverloads constructor(
    val layout: Layout,
    val state: State?,
    override val analyticsInfo: AnalyticsInfo,
    val isExpectsRequest: Boolean = false,
    val actions: Map<String, ActionRef> = mapOf(),
    val stackEngine: StackEngine? = null,
    override val serverDirectives: List<ServerDirective> = listOf(),
    val scenarioData: TScenarioData? = null,
    override val actionSpaces: Map<String, ActionSpace> = mapOf(),
    val renderData: List<DivRenderData> = listOf()
) : IScenarioResponseBody<State> {
    // constructor for old java calls
    @JvmOverloads
    constructor(
        layout: Layout,
        state: Optional<State>,
        analyticsInfo: AnalyticsInfo,
        isExpectsRequest: Boolean = false,
        actions: Map<String, ActionRef> = mapOf(),
        serverDirectives: List<ServerDirective> = listOf(),
        stackEngine: StackEngine? = null,
        actionSpaces: Map<String, ActionSpace> = mapOf(),
        renderData: List<DivRenderData> = listOf()
    ) :
        this(layout, state.orElse(null), analyticsInfo, isExpectsRequest, actions, stackEngine,
            serverDirectives, null, actionSpaces, renderData)

    @JvmOverloads
    constructor(
        layout: Layout,
        analyticsInfo: AnalyticsInfo,
        isExpectsRequest: Boolean = false,
        actions: Map<String, ActionRef> = mapOf()
    ) :
        this(layout, null, analyticsInfo, isExpectsRequest, actions)
}

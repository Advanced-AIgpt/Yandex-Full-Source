package ru.yandex.alice.kronstadt.core

import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfo
import ru.yandex.alice.kronstadt.core.directive.server.ServerDirective

sealed interface IScenarioResponseBody<State> {
    val analyticsInfo: AnalyticsInfo
    val serverDirectives: List<ServerDirective>
    val actionSpaces: Map<String, ActionSpace>
}

package ru.yandex.alice.kronstadt.core

import com.google.protobuf.Message
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfo
import ru.yandex.alice.kronstadt.core.directive.server.ServerDirective

data class GrpcResponseBody<State>(
    val grpcResponse: Message,
    override val analyticsInfo: AnalyticsInfo,
    override val serverDirectives: List<ServerDirective> = listOf(),
    override val actionSpaces: Map<String, ActionSpace> = mapOf(),
) : IScenarioResponseBody<State> {
}

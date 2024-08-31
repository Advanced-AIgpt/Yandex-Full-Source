package ru.yandex.alice.kronstadt.core.analytics

import ru.yandex.alice.kronstadt.core.IrrelevantResponse
import kotlin.contracts.ExperimentalContracts
import kotlin.contracts.InvocationKind
import kotlin.contracts.contract

open class AnalyticsInfo(
    val intent: String,
    val actions: List<AnalyticsInfoAction> = listOf(),
    val objects: List<AnalyticsInfoObject> = listOf(),
    val events: List<AnalyticsInfoEvent> = listOf(),
) {

    companion object {
        @JvmField
        val IRRELEVANT = AnalyticsInfo(intent = IrrelevantResponse.IRRELEVANT)
    }
}

class AnalyticsInfoBuilder internal constructor(
    private val actions: MutableList<AnalyticsInfoAction> = mutableListOf(),
    private val objects: MutableList<AnalyticsInfoObject> = mutableListOf(),
    private val events: MutableList<AnalyticsInfoEvent> = mutableListOf(),
) {
    fun action(action: AnalyticsInfoAction) = actions.add(action)
    fun obj(obj: AnalyticsInfoObject) = objects.add(obj)
    fun event(event: AnalyticsInfoEvent) = events.add(event)

    internal fun build(intent: String): AnalyticsInfo {
        return AnalyticsInfo(intent, actions, objects, events)
    }
}

@OptIn(ExperimentalContracts::class)
fun analyticsInfo(intent: String, init: AnalyticsInfoBuilder.() -> Unit): AnalyticsInfo {
    contract { callsInPlace(init, InvocationKind.EXACTLY_ONCE) }
    return AnalyticsInfoBuilder().apply(init).build(intent)
}

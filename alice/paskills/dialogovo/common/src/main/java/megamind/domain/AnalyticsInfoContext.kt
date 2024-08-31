package ru.yandex.alice.paskill.dialogovo.megamind.domain

import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfo
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfoAction
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfoEvent
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfoObject
import java.util.concurrent.ConcurrentLinkedQueue

const val DEFAULT_INTENT = "unknown"

class AnalyticsInfoContext {
    private val intents = ConcurrentLinkedQueue<String>()
    private val actions = ConcurrentLinkedQueue<AnalyticsInfoAction>()
    private val objects = ConcurrentLinkedQueue<AnalyticsInfoObject>()
    private val events = ConcurrentLinkedQueue<AnalyticsInfoEvent>()
    private var intent = DEFAULT_INTENT
    fun setIntent(intent: String): AnalyticsInfoContext {
        intents.add(intent)
        this.intent = intent
        return this
    }

    fun addEvent(event: AnalyticsInfoEvent): AnalyticsInfoContext {
        events.add(event)
        return this
    }

    fun addObject(obj: AnalyticsInfoObject): AnalyticsInfoContext {
        objects.add(obj)
        return this
    }

    fun addAction(action: AnalyticsInfoAction): AnalyticsInfoContext {
        actions.add(action)
        return this
    }

    fun toAnalyticsInfo(intent: String): AnalyticsInfo {
        return setIntent(intent).toAnalyticsInfo()
    }

    fun toAnalyticsInfo(): AnalyticsInfo {
        return AnalyticsInfo(intent, actions.toList(), objects.toList(), events.toList())
    }
}

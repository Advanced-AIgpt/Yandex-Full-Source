package ru.yandex.alice.kronstadt.core.log

import org.apache.logging.log4j.ThreadContext
import ru.yandex.alice.kronstadt.core.log.LoggingContext.Companion.builder

/**
 * Utility logging context abstraction over log provider
 */
class LoggingContextHolder private constructor() {
    companion object {
        private val KEYS = LoggingContext.Key.values().map { it.key }.toList()

        @JvmStatic
        var current: LoggingContext
            get() = builder()
                .contextMap(LoggingContext.Key.values().associateWith { ThreadContext.get(it.key) })
                .build()
            set(context) {
                context.contextMap.forEach { (key: LoggingContext.Key, value: String?) ->
                    ThreadContext.put(key.key, value)
                }
            }

        @JvmStatic
        fun clearCurrent() {
            ThreadContext.removeAll(KEYS)
        }
    }

    init {
        throw UnsupportedOperationException()
    }
}

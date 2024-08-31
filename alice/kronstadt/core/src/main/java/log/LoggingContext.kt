package ru.yandex.alice.kronstadt.core.log

import java.io.Serializable
import java.util.EnumMap

/**
 * Persistent thread context
 */
class LoggingContext private constructor(builder: Builder) : Serializable {
    val contextMap: Map<Key, String?>

    enum class Key(internal val key: String) {
        UID("uid"), REQUEST_ID("request_id");
    }

    class Builder {
        internal val contextMap: MutableMap<Key, String?> = EnumMap(Key::class.java)

        fun contextMap(contextMap: Map<Key, String>): Builder {
            this.contextMap.putAll(contextMap)
            return this
        }

        fun user(uid: String?): Builder {
            contextMap[Key.UID] = uid
            return this
        }

        fun requestId(requestId: String?): Builder {
            contextMap[Key.REQUEST_ID] = requestId
            return this
        }

        fun build(): LoggingContext {
            return LoggingContext(this)
        }
    }

    companion object {
        @JvmStatic
        fun builder(): Builder {
            return Builder()
        }
    }

    init {
        contextMap = builder.contextMap
    }
}

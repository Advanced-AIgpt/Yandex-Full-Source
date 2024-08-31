package ru.yandex.alice.paskill.dialogovo.service.state

import com.fasterxml.jackson.annotation.JsonIgnore
import com.fasterxml.jackson.annotation.JsonProperty
import java.time.Instant
import java.util.Optional

data class SkillState constructor(
    val user: User?,
    val session: Session?,
    val application: Application?
) {

    fun getUserState() = user?.state ?: emptyMap()
    fun getSessionState() = session?.state ?: emptyMap()
    fun getApplicationState() = application?.state ?: emptyMap()

    /**
     * similar to UserState but for public use
     */
    interface User {
        @get:JsonIgnore
        val timestamp: Instant?

        @get:JsonProperty("timestamp")
        val timestampLong: Long
            get() = Optional.ofNullable(timestamp).map { obj: Instant -> obj.toEpochMilli() }
                .orElse(0L)
        val state: Map<String, Any>
    }

    interface Session {
        @get:JsonIgnore
        val timestamp: Instant?

        @get:JsonProperty("timestamp")
        val timestampLong: Long
            get() = Optional.ofNullable(timestamp).map { obj: Instant -> obj.toEpochMilli() }
                .orElse(0L)
        val state: Map<String, Any>
    }

    interface Application {
        @get:JsonIgnore
        val timestamp: Instant?

        @get:JsonProperty("timestamp")
        val timestampLong: Long
            get() = Optional.ofNullable(timestamp).map { obj: Instant -> obj.toEpochMilli() }
                .orElse(0L)
        val state: Map<String, Any>
    }

    companion object {
        @JvmField
        val EMPTY = SkillState(null, null, null)

        @JvmStatic
        fun create(user: User? = null, session: Session? = null, application: Application? = null): SkillState {
            return if (user == null && session == null && application == null) EMPTY else SkillState(
                user,
                session,
                application
            )
        }
    }
}

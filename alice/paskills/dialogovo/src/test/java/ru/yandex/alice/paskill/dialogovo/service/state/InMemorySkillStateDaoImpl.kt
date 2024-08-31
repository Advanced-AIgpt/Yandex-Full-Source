package ru.yandex.alice.paskill.dialogovo.service.state

import ru.yandex.alice.paskill.dialogovo.service.state.SkillState.Companion.create
import java.time.Instant
import java.util.concurrent.CompletableFuture

class InMemorySkillStateDaoImpl : SkillStateDao {
    val userStates: MutableMap<UserStateKey, UserStateEntity> = LinkedHashMap()
    val sessionStates: MutableMap<SessionStateKey, SessionStateEntity> = LinkedHashMap()
    val applicationStates: MutableMap<ApplicationStateKey, ApplicationStateEntity> = LinkedHashMap()

    val users: List<UserStateEntity>
        get() = userStates.map { entry ->
            UserStateEntity(entry.key.skillId, entry.key.userId, entry.value.timestamp, entry.value.state)
        }
    val sessions: List<SessionStateEntity>
        get() = sessionStates.map { entry ->
            SessionStateEntity(entry.key.skillId, entry.key.sessionId, entry.value.timestamp, entry.value.state)
        }

    val applications: List<ApplicationStateEntity>
        get() = applicationStates.map { entry ->
            ApplicationStateEntity(entry.key.skillId, entry.key.applicationId, entry.value.timestamp, entry.value.state)
        }

    fun load(
        sessions: List<SessionStateEntity>,
        users: List<UserStateEntity>,
        applications: List<ApplicationStateEntity>
    ) {
        sessions.forEach { session: SessionStateEntity ->
            val key = SessionStateKey(session.sessionId, session.skillId)
            sessionStates[key] = session
        }

        users.forEach { user: UserStateEntity ->
            val key = UserStateKey(user.userId, user.skillId)
            userStates[key] = user
        }

        applications.forEach { application: ApplicationStateEntity ->
            val key = ApplicationStateKey(application.applicationId, application.skillId)
            applicationStates[key] = application
        }
    }

    fun clear() {
        userStates.clear()
        sessionStates.clear()
        applicationStates.clear()
    }

    override fun findBySkillIdAndUserIdAndSessionIdAndApplicationId(skillStateId: SkillStateId): SkillState {
        val userId = skillStateId.userId
        val deviceId = skillStateId.applicationId
        val sessionId = skillStateId.sessionId
        val skillId = skillStateId.skillId
        val userState = if (userId != null) userStates[UserStateKey(userId, skillId)] else null
        val sessionState = sessionStates[SessionStateKey(sessionId, skillId)]
        val applicationState = applicationStates[ApplicationStateKey(deviceId, skillId)]
        return create(userState, sessionState, applicationState)
    }

    override fun findBySkillIdAndUserIdAndSessionIdAndApplicationIdAsync(skillStateId: SkillStateId) =
        CompletableFuture.supplyAsync { findBySkillIdAndUserIdAndSessionIdAndApplicationId(skillStateId) }

    override fun storeSessionAndUserAndApplicationState(
        skillStateId: SkillStateId,
        requestTime: Instant,
        sessionState: Map<String, Any>?,
        userStateIncrement: Map<String, Any?>,
        applicationState: Map<String, Any>?
    ) {
        val userId = skillStateId.userId
        val applicationId = skillStateId.applicationId
        val sessionId = skillStateId.sessionId
        val skillId = skillStateId.skillId

        incrementSessionState(sessionId, skillId, sessionState)
        if (userId != null && userStateIncrement.isNotEmpty()) {
            incrementUserState(skillId, userId, userStateIncrement)
        }

        applicationState?.let { stringObjectMap: Map<String, Any> ->
            incrementApplicationState(skillId, applicationId, stringObjectMap)
        }
    }

    private fun incrementSessionState(sessionId: String, skillId: String, sessionState: Map<String, Any>?) {
        val key = SessionStateKey(sessionId, skillId)
        if (sessionState != null) {
            if (sessionState.isNotEmpty()) {
                sessionStates[key] = SessionStateEntity(skillId, sessionId, Instant.now(), sessionState)
            } else {
                sessionStates.remove(key)
            }
        }
    }

    private fun incrementUserState(skillId: String, userId: String, userStateIncrement: Map<String, Any?>) {
        val key1 = UserStateKey(userId, skillId)

        val oldState: UserStateEntity = userStates.getOrElse(key1) {
            UserStateEntity(skillId, userId, Instant.now(), emptyMap())
        }

        val stateMap = LinkedHashMap(oldState.state)

        userStateIncrement.forEach { (key: String, value: Any?) ->
            stateMap.compute(key) { _, _ -> value }
        }
        userStates[key1] = oldState.copy(state = stateMap, timestamp = Instant.now())
    }

    private fun incrementApplicationState(
        skillId: String,
        applicationId: String,
        applicationState: Map<String, Any>
    ) {
        val key1 = ApplicationStateKey(applicationId, skillId)
        if (applicationState.isNotEmpty()) {
            applicationStates[key1] = ApplicationStateEntity(skillId, applicationId, Instant.now(), applicationState)
        } else {
            applicationStates.remove(key1)
        }
    }

    data class UserStateKey(val userId: String, val skillId: String)
    data class SessionStateKey(val sessionId: String, val skillId: String)
    data class ApplicationStateKey(val applicationId: String, val skillId: String)
}

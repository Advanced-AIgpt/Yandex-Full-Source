package ru.yandex.alice.paskill.dialogovo.service.state

import java.time.Instant
import java.util.concurrent.CompletableFuture

/**
 * User state change is implemented via incremental approach:
 * skill responses with an increment and only entries specified are changed/removed
 */
interface SkillStateDao {
    fun findBySkillIdAndUserIdAndSessionIdAndApplicationId(skillStateId: SkillStateId): SkillState
    fun findBySkillIdAndUserIdAndSessionIdAndApplicationIdAsync(skillStateId: SkillStateId): CompletableFuture<SkillState>

    /**
     * update/delete session state (remove existing key if null value provided) and update user state
     *
     * @param skillStateId       (skill identifier, skill session identifier, user uid/uuid, application id)
     * @param requestTime        request time instant
     * @param sessionState       session state to be stored as-is. if null - won't be stored
     * @param userStateIncrement user state increment. if an entry has null value, the existing key will be
     * deleted.
     * @param applicationState   application state. if the value is empty map, the existing state wiil be deleted
     */
    fun storeSessionAndUserAndApplicationState(
        skillStateId: SkillStateId,
        requestTime: Instant,
        sessionState: Map<String, Any>?,
        userStateIncrement: Map<String, Any?>,
        applicationState: Map<String, Any>?
    )
}

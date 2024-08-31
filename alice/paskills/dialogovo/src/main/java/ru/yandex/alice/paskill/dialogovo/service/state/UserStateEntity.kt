package ru.yandex.alice.paskill.dialogovo.service.state

import java.time.Instant

/**
 * YDB table entity
 */
data class UserStateEntity(
    val skillId: String, // uuid for anonymous or uid for real user
    val userId: String,
    override val timestamp: Instant?, // user global state map
    override val state: Map<String, Any> = emptyMap()
) : SkillState.User

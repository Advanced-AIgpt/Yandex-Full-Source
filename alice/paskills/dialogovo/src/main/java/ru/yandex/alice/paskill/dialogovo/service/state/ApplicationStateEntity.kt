package ru.yandex.alice.paskill.dialogovo.service.state

import java.time.Instant

/**
 * YDB table entity
 */
data class ApplicationStateEntity(
    val skillId: String,
    val applicationId: String,
    override val timestamp: Instant?,
    // json payload
    override val state: Map<String, Any>
) : SkillState.Application

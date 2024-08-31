package ru.yandex.alice.paskill.dialogovo.service.state

data class SkillStateId(
    val skillId: String,
    val userId: String?,
    val sessionId: String,
    val applicationId: String
)

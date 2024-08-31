package ru.yandex.alice.paskill.dialogovo.config

data class UserSkillProductConfig(
    val musicIdToTokenCode: Map<String, String>,
    val musicUrlToTokenCode: Map<String, String>
)

package ru.yandex.alice.social.sharing.apphost.handlers.document.stateless

internal data class Secret(
    val value: String,
)

internal data class SkillSecretResponse(
    val secrets: List<Secret>,
)

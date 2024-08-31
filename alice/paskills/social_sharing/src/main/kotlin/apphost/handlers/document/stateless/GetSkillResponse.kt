package ru.yandex.alice.social.sharing.apphost.handlers.document.stateless

internal data class SkillLogo(
    val avatarId: String,
    val color: String,
)

internal data class SkillInfo(
    val id: String,
    val logo: SkillLogo,
    val name: String,
    val averageRating: Double,
)

package ru.yandex.alice.kronstadt.core.domain

data class BlackboxInfo(
    val uid: String,
    val email: String,
    val firstName: String,
    val lastName: String,
    val phone: String,
    val hasYandexPlus: Boolean,
    val isStaff: Boolean,
    val isBetaTester: Boolean,
    val hasMusicSubscription: Boolean,
)

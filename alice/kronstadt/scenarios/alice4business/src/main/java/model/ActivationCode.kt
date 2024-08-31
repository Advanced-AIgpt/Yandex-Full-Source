package ru.yandex.alice.kronstadt.scenarios.alice4business.model

import org.springframework.data.annotation.Id
import java.sql.Timestamp
import java.util.UUID

data class ActivationCode(
    @Id
    val code: String,
    val deviceId: UUID,
    val createdAt: Timestamp
)

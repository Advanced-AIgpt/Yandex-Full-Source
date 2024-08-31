package ru.yandex.alice.kronstadt.scenarios.alice4business.model

import org.springframework.data.annotation.Id
import java.util.UUID

data class BusinessDeviceWithStatus(
    @Id
    val id: UUID,
    val platform: String,
    val status: DeviceStatus
)

enum class DeviceStatus(val dbName: String) {
    Active("active"),
    Inactive("inactive"),
    Reset("reset"),
    Unknown(""),
}

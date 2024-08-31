package ru.yandex.alice.kronstadt.scenarios.alice4business.model.dto

internal data class GetDevicesListResponse(
    val result: List<DeviceIdentifier>?
)

internal data class DeviceIdentifier(
    val platform: String,
    val deviceId: String,
)

package ru.yandex.alice.kronstadt.scenarios.alice4business

internal val UNLOCKED_DEVICE_STATE = DeviceLockState()

data class DeviceLockState(
    val locked: Boolean = false,
    val stationUrl: String? = null,
    val code: String? = null,
)

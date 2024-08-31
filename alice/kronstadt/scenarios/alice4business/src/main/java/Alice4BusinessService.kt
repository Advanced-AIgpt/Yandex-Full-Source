package ru.yandex.alice.kronstadt.scenarios.alice4business

import java.time.Instant

interface Alice4BusinessService {
    fun isBusinessDevice(deviceId: String?): Boolean
    fun getDeviceLockState(deviceId: String?, requestTime: Instant): DeviceLockState?
    val isReady: Boolean
}

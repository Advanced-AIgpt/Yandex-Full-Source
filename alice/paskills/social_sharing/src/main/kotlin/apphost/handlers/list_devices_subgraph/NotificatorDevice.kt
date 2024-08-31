package ru.yandex.alice.social.sharing.apphost.handlers.list_devices

import com.fasterxml.jackson.annotation.JsonProperty

internal data class NotificatorDevice(
    @JsonProperty("device_id") val deviceId: String,
)

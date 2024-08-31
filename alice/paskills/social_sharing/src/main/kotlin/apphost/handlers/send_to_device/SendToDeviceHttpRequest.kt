package ru.yandex.alice.social.sharing.apphost.handlers.send_to_device

import com.fasterxml.jackson.annotation.JsonProperty

data class SendToDeviceHttpRequest(
    @JsonProperty("device_id") val deviceId: String,
)


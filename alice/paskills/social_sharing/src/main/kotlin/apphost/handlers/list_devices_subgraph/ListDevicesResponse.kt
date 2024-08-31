package ru.yandex.alice.social.sharing.apphost.handlers.list_devices

import com.fasterxml.jackson.annotation.JsonProperty

internal data class ListDevicesResponse(
        @JsonProperty("devices") val devices: List<Device>,
) {
    data class Device(
        @JsonProperty("device_id") val deviceId: String,
        @JsonProperty("room") val room: String?,
        @JsonProperty("type") val type: String,
        @JsonProperty("icon_url") val iconUrl: String,
        @JsonProperty("name") val name: String,
    ) {
        constructor(notificatorDevice: NotificatorDevice, iotDevice: IotDevice, room: Room?) : this(
            notificatorDevice.deviceId,
            room?.name,
            iotDevice.type!!,
            iotDevice.iconUrl,
            iotDevice.name
        )
    }

}

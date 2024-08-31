package ru.yandex.alice.social.sharing.apphost.handlers.list_devices

import com.fasterxml.jackson.annotation.JsonIgnore
import com.fasterxml.jackson.annotation.JsonProperty

internal data class IotResponse(
    val status: String,
    @JsonProperty("request_id") val requestId: String?,
    val payload: IotResponsePayload?,
)

internal data class IotResponsePayload(
    @JsonProperty("devices") val devices: List<IotDevice>,
    @JsonProperty("rooms") private val roomList: List<Room>
) {
    @JsonIgnore
    val rooms: Map<String, Room> = roomList.map { room -> room.id to room }.toMap()

    @JsonIgnore
    val quasarDevices: Map<String, IotDevice> = devices
        .filter { it.quasarInfo != null && it.type != null }
        .map { d -> d.quasarInfo!!.deviceId to d }.toMap()
}

internal data class IotDevice(
    val id: String,
    @JsonProperty("external_id") val externalId: String?,
    val name: String,
    @JsonProperty("room_id") val roomId: String?,
    val type: String?,
    @JsonProperty("quasar_info") val quasarInfo: QuasarInfo?,
    @JsonProperty("icon_url") val iconUrl: String,
)

internal data class QuasarInfo(
    @JsonProperty("device_id") val deviceId: String,
    val platform: String,
)

internal data class Room(
    val id: String,
    val name: String,
)


package ru.yandex.alice.social.sharing.apphost.handlers.send_to_device

import NMatrix.NApi.Delivery
import com.fasterxml.jackson.annotation.JsonValue

enum class SendToDeviceStatus(
    @JsonValue val value: String
) {
    OK("OK"),
    BAD_REQUEST("BAD_REQUEST"),
    DOCUMENT_NOT_FOUND("DOCUMENT_NOT_FOUND"),
    NO_ONLINE_DEVICES("NO_ONLINE_DEVICES"),
    UNKNOWN_ERROR("UNKNOWN_ERROR");

    companion object {
        fun fromNotificatorResponseCode(
            notificatorResponseCode: Delivery.TDeliveryResponse.EResponseCode,
        ): SendToDeviceStatus {
            return when(notificatorResponseCode) {
                Delivery.TDeliveryResponse.EResponseCode.Unknown -> UNKNOWN_ERROR
                Delivery.TDeliveryResponse.EResponseCode.OK -> OK
                Delivery.TDeliveryResponse.EResponseCode.NoLocations -> NO_ONLINE_DEVICES
                Delivery.TDeliveryResponse.EResponseCode.UNRECOGNIZED -> UNKNOWN_ERROR
            }
        }
    }
}

class SendToDeviceResponseBody(
    val status: SendToDeviceStatus,
)

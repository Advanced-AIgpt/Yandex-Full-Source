package ru.yandex.alice.social.sharing.apphost.handlers.list_devices

import NAlice.NNotificator.Api
import NAppHostHttp.Http
import com.fasterxml.jackson.databind.ObjectMapper
import org.apache.logging.log4j.LogManager
import org.springframework.stereotype.Component
import ru.yandex.alice.social.sharing.ApphostHandler
import ru.yandex.alice.social.sharing.apphost.handlers.LIST_DEVICES_RESPONSE
import ru.yandex.alice.social.sharing.proto.WebApi
import ru.yandex.alice.social.sharing.proto.context.ListDevicesProto
import ru.yandex.web.apphost.api.request.RequestContext

@Component
class ListDevicesRenderApphostResponse(
        private val objectMapper: ObjectMapper,
): ApphostHandler {
    override val path = "/list_devices/render_apphost_response"

    override fun handle(context: RequestContext) {
        val notificatorHttpResponse = context
            .getSingleRequestItem("notificator_device_list_http_response")
            .getProtobufData(Http.THttpResponse.getDefaultInstance())
        val notificatorDeviceListResponse = Api.TGetDevicesResponse.parseFrom(notificatorHttpResponse.content)
        val onlineDevices: List<NotificatorDevice> = notificatorDeviceListResponse.devicesList
            .map { NotificatorDevice(it.deviceId) }

        val iotHttpResponse = context.getSingleRequestItem("iot_device_list_http_response")
            .getProtobufData(Http.THttpResponse.getDefaultInstance())
        val iotResponse = objectMapper.readValue(iotHttpResponse.content.toByteArray(), IotResponse::class.java)
        val iotDevices = iotResponse.payload?.quasarDevices ?: emptyMap()
        val rooms = iotResponse.payload?.rooms ?: emptyMap()
        logger.debug("IoT response:\n{}", iotResponse)

        val devices: List<ListDevicesResponse.Device> = onlineDevices
            .filter { iotDevices.containsKey(it.deviceId) }
            .map {
                val iotDevice = iotDevices[it.deviceId]!!
                val room = rooms[iotDevice.roomId]
                ListDevicesResponse.Device(it, iotDevice, room)
            }

        logger.info("IoT devices: {}\nNotificator devices: {}\nMerged devices: {}",
            iotDevices.map { d -> d.value.quasarInfo?.deviceId },
            onlineDevices.map { d -> d.deviceId },
            devices.map { d -> d.deviceId },
        )

        context.addProtobufItem(
                LIST_DEVICES_RESPONSE,
            ListDevicesProto.TListDevicesResponse.newBuilder()
                .addAllDevices(devices.map {
                    WebApi.TWebPageTemplateData.TDevice.newBuilder()
                        .setDeviceId(it.deviceId)
                        .setRoom(it.room.orEmpty())
                        .setType(it.type)
                        .setIconUrl(it.iconUrl)
                        .setName(it.name)
                        .build()
                })
                .build()
        )
    }

    companion object {
        private val logger = LogManager.getLogger()
    }
}

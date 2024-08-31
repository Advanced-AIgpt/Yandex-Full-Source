package ru.yandex.alice.paskill.dialogovo.utils

import org.apache.logging.log4j.LogManager
import ru.yandex.alice.kronstadt.core.domain.ClientInfo
import ru.yandex.alice.kronstadt.core.domain.Interfaces
import java.util.regex.Pattern

private val PATTERN = Pattern.compile(
    "^(?<appId>.+?)/(?<appVersion>.+?)\\s\\((?<deviceManufacturer>.+?)\\s(?<deviceModel>.+?);" +
        "\\s(?<platform>.+?)\\s(?<osVersion>.+?)\\)$"
)

private val logger = LogManager.getLogger()

fun parseClientInfo(clientId: String?, uuid: String, interfaces: Interfaces): ClientInfo {

    val default = ClientInfo(
        appId = "ru.yandex.searchplugin",
        appVersion = "7.16",
        deviceManufacturer = "none",
        deviceModel = "none",
        platform = "android",
        osVersion = "4.4.2",
        uuid = uuid,
        interfaces = interfaces,
    )
    if (clientId.isNullOrEmpty()) {
        return default
    }

    val matcher = PATTERN.matcher(clientId)
    if (matcher.find()) {
        try {
            return ClientInfo(
                appId = matcher.group("appId"),
                appVersion = matcher.group("appVersion"),
                deviceManufacturer = matcher.group("deviceManufacturer"),
                deviceModel = matcher.group("deviceModel"),
                deviceId = "fakedeviceid",
                platform = matcher.group("platform"),
                osVersion = matcher.group("osVersion"),
                uuid = uuid,
                interfaces = interfaces
            )
        } catch (e: Exception) {
            logger.warn("Unable to parse client_id: {}", clientId)
        }
    }
    return default
}

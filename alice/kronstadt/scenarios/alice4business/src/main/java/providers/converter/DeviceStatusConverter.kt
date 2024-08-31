package ru.yandex.alice.kronstadt.scenarios.alice4business.providers.converter

import org.apache.logging.log4j.LogManager
import org.springframework.core.convert.converter.Converter
import org.springframework.data.convert.ReadingConverter
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.scenarios.alice4business.model.DeviceStatus

@Component
@ReadingConverter
class DeviceStatusConverter : Converter<String, DeviceStatus> {
    override fun convert(source: String): DeviceStatus {
        return DeviceStatus.values().find { it.dbName == source }
            ?: DeviceStatus.Unknown.also {
                logger.error("Unknown device status: $source")
            }
    }

    companion object {
        private val logger = LogManager.getLogger(DeviceStatusConverter::class.java)
    }
}

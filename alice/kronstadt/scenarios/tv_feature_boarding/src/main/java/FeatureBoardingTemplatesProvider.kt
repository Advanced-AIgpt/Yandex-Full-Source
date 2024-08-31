package ru.yandex.alice.kronstadt.scenarios.tv_feature_boarding

import com.google.protobuf.Struct
import com.yandex.div.dsl.serializer.toJsonNode
import org.springframework.core.io.support.PathMatchingResourcePatternResolver
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.convert.ProtoUtil
import ru.yandex.alice.divkttemplates.feature_boarding.NoTandemDevicePromoTemplate.getNoTandemDevicePromo
import ru.yandex.alice.divkttemplates.feature_boarding.TandemDeviceAvailablePromoTemplate.getTandemDeviceAvailablePromo

const val DIV_PROMO_FOLDER = "div_promo"

@Component
class FeatureBoardingTemplatesProvider(private val protoUtil: ProtoUtil) {

    private val resolver = PathMatchingResourcePatternResolver()

    val noTandemDevicePromo = protoUtil.objectNodeToStruct(getNoTandemDevicePromo().toJsonNode())

    fun getTandemDeviceAvailablePromo(version: String): Struct {
        if (version == "new") {
            return protoUtil.objectNodeToStruct(getTandemDeviceAvailablePromo().toJsonNode())
        } else {
            return "$DIV_PROMO_FOLDER/tandem_device_available_promo.json".toStruct()
        }
    }

    private fun String.toStruct(): Struct {
        val resource = resolver.getResource(this)
        if (!resource.exists()) {
            throw RuntimeException("Resource at path '$this' does not exists")
        }
        return protoUtil.jsonStringToStruct(String(resource.inputStream.readBytes()))
    }
}

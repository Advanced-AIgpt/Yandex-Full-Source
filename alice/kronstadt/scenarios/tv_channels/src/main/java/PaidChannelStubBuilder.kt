package ru.yandex.alice.kronstadt.scenarios.tv_channels

import org.apache.logging.log4j.LogManager
import ru.yandex.alice.kronstadt.scenarios.tv_channels.model.PaidChannelStubV2
import ru.yandex.alice.kronstadt.scenarios.tv_channels.vh.response.OttLicense
import ru.yandex.alice.kronstadt.scenarios.tv_channels.vh.response.OttParams

object PaidChannelStubBuilder {

    private val logger = LogManager.getLogger()

    const val PLUS_TAG = "plus"
    private val COMMON_PLACEHOLDERS = mapOf(
        "plus" to "https://avatars.yandex.net/get-dialogs/1530877/plus/orig",
        "amedia" to "https://avatars.yandex.net/get-dialogs/1535439/plus_amediateka/orig",
        "more" to "https://avatars.yandex.net/get-dialogs/1676983/plus_more/orig",
    )
    private val PLUS_STUB = PaidChannelStubV2(
        header = "Нужен\nЯндекс Плюс",
        text = "Более 100 телеканалов, фильмы\nи сериалы со знаком {plus}, а ещё\nмузыка и подкасты.",
        deeplink = "live-tv://try-for-free",
        background = "https://avatars.mds.yandex.net/get-dialogs/998463/plus/orig",
        placeholders = COMMON_PLACEHOLDERS,
    )

    const val AMEDIATEKA_TAG = "kp-amediateka"
    private val AMEDIATEKA_STUB = PaidChannelStubV2(
        header = "Нужен Плюс Мульти\nс Амедиатекой",
        text = "Телеканалы, фильмы и сериалы\nсо знаками {amedia} и {more}. А ещё\nмузыка, подкасты и кешбэк\nбаллами.",
        deeplink = "live-tv://try-for-free",
        background = "https://avatars.mds.yandex.net/get-dialogs/1676983/amedia/orig",
        placeholders = COMMON_PLACEHOLDERS,
    )

    fun getStub(ottParams: OttParams?): PaidChannelStubV2? {
        if (ottParams == null || ottParams.licenses.isNullOrEmpty()) {
            logger.warn("Licenses list is empty or null, can not get stub for ${ottParams}")
            return null
        }

        val license = LicenseUtils.findPrimary(ottParams.licenses)

        if (license == null) {
            logger.warn("No primary license, can not get stub for ${ottParams}")
            return null
        }

        if (LicenseUtils.isActive(license)) {
            logger.info("User has active license for this content, stub is not required")
            return null
        }

        return getStubForPurchaseTag(license.purchaseTag)
    }

    private fun getStubForPurchaseTag(purchaseTag: String?): PaidChannelStubV2 {
        return when (purchaseTag) {
            PLUS_TAG -> {
                PLUS_STUB
            }
            AMEDIATEKA_TAG -> {
                AMEDIATEKA_STUB
            }
            else -> {
                logger.error("Can not find stub for purchase tag: ${purchaseTag}, using Plus as fallback")
                PLUS_STUB
            }
        }
    }
}

object LicenseUtils {
    fun findPrimary(licenses: List<OttLicense>): OttLicense? = licenses.find { it.primary }
    fun isActive(license: OttLicense): Boolean = license.active == true
}

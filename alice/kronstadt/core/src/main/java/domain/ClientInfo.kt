package ru.yandex.alice.kronstadt.core.domain

import java.util.Locale
import java.util.Optional
import kotlin.math.max

// actual at 5823372
private val APP_ID_TO_SURFACE_MAPPING: Map<String, Surface> = mapOf(
    "ru.yandex.searchplugin" to Surface.MOBILE,
    "ru.yandex.mobile" to Surface.MOBILE,
    "ru.yandex.searchplugin.beta" to Surface.MOBILE,
    "ru.yandex.mobile.search" to Surface.DESKTOP,
    "ru.yandex.mobile.search.ipad" to Surface.DESKTOP,
    "winsearchbar" to Surface.DESKTOP,
    "com.yandex.browser" to Surface.DESKTOP,
    "com.yandex.browser.alpha" to Surface.DESKTOP,
    "com.yandex.browser.beta" to Surface.DESKTOP,
    "ru.yandex.yandexnavi" to Surface.NAVIGATOR,
    "ru.yandex.mobile.navigator" to Surface.NAVIGATOR,
    "ru.yandex.yandexmaps" to Surface.NAVIGATOR,
    "ru.yandex.traffic" to Surface.NAVIGATOR,
    "com.yandex.launcher" to Surface.MOBILE,
    "ru.yandex.quasar.services" to Surface.QUASAR,
    "yandex.auto" to Surface.AUTO,
    "ru.yandex.autolauncher" to Surface.AUTO,
    "ru.yandex.iosdk.elariwatch" to Surface.WATCH,
    "aliced" to Surface.QUASAR,
    "yabro" to Surface.DESKTOP,
    "yabro.beta" to Surface.DESKTOP,
    "ru.yandex.quasar.app" to Surface.QUASAR,
    "yandex.auto.old" to Surface.AUTO,
    "ru.yandex.mobile.music" to Surface.MOBILE,
    "ru.yandex.music" to Surface.MOBILE,
    "ru.yandex.centaur" to Surface.CENTAUR,
)

class ClientInfo(
    appId: String? = null,
    appVersion: String? = null,
    platform: String? = null,
    osVersion: String? = null,
    uuid: String? = null,
    deviceId: String? = null,
    lang: String? = null,
    timezone: String? = null,
    deviceModel: String? = null,
    deviceManufacturer: String? = null,
    interfaces: Interfaces = Interfaces()
) {
    val appId: String = trimToNone(appId).lowercase(Locale.ROOT)
    val appVersion: String = trimToNone(appVersion)
    val platform: String = trimToNone(platform)
    val osVersion: String = trimToNone(osVersion)
    val uuid: String = trimToNone(uuid)
    val deviceId: String? = deviceId
    val deviceIdO: Optional<String>
        get() = Optional.ofNullable(deviceId)
    val lang: String = trimToValue(lang, "ru-RU")
    val timezone: String = trimToValue(timezone, "UTC")
    val deviceModel: String = trimToNone(deviceModel)
    val deviceManufacturer: String = trimToNone(deviceManufacturer)
    val appSemVersion: Version? = createSemVersion(this.appVersion)
    val iOsVersion: Int? = parseIOSAppVersion(this.appVersion)
    val osSemVersion: Version? = createSemVersion(this.osVersion)
    val interfaces: Interfaces = interfaces

    private fun parseIOSAppVersion(appVersion: String): Int? {
        return try {
            appVersion.substring(appVersion.indexOf(":")).toInt()
        } catch (e: Exception) {
            null
        }
    }

    private fun trimToNone(str: String?): String {
        return trimToValue(str, "none")
    }

    private fun trimToValue(origStr: String?, defaultValue: String): String {
        return origStr
            ?.trim()
            ?.takeIf { it.isNotEmpty() && "null" != it }
            ?: defaultValue
    }

    // https://wiki.yandex-team.ru/dialogs/development/station/#poleinterfaces
    // https://a.yandex-team.ru/arc/trunk/arcadia/alice/bass/forms/external_skill/ifs_map.cpp?rev=5559348#L20
    val isHasScreenBASSimpl: Boolean
        get() = !isSmartSpeaker && !isElariWatch && !isNavigatorOrMaps && !isYaAuto
    val isSupportsBilling: Boolean
        get() = (isSmartSpeaker
            || isSearchApp
            || isTvDevice && interfaces.canServerAction && interfaces.hasSynchronizedPush)
    val isSupportsAccountLinking: Boolean
        get() = (isYaBrowserDesktop
            || isSmartSpeaker
            || isSearchApp
            || isYaBrowserMobile
            || isTvDevice && interfaces.canServerAction && interfaces.hasSynchronizedPush)
    val isSupportPurchase: Boolean
        get() = (isYaBrowserDesktop
            || isSmartSpeaker
            || isSearchApp
            || isYaBrowserMobile
            || isTvDevice && interfaces.canServerAction && interfaces.hasSynchronizedPush)
    val isSupportMusicActivation: Boolean
        get() = isSmartSpeaker || isSearchApp
    val isSupportGeolocationSharing: Boolean
        get() = isSearchApp || isYaBrowserMobile
    val isSupportUserAgreements: Boolean
        get() = (isYaBrowserDesktop
            || isSmartSpeaker
            || isSearchApp
            || isYaBrowserMobile
            || isTvDevice && interfaces.canServerAction && interfaces.hasSynchronizedPush)
    val isAndroid: Boolean
        get() = "android".equals(platform, ignoreCase = true) && !isQuasar
    val isIOS: Boolean
        get() = "ios".equals(platform, ignoreCase = true) ||
            "ipad".equals(platform, ignoreCase = true) ||
            "iphone".equals(platform, ignoreCase = true)
    val isWindows: Boolean
        get() = "windows".equals(platform, ignoreCase = true)
    val isLinux: Boolean
        get() = "linux".equals(platform, ignoreCase = true)
    val isTestClient: Boolean
        get() = appId == "uniproxy.monitoring" || appId == "uniproxy.test" || appId == "com.yandex.search.shooting" || appId == "test" ||
            appId.startsWith("com.yandex.vins")
    val isQuasar: Boolean
        get() = appId.startsWith("ru.yandex.quasar")
    val isMiniSpeaker: Boolean
        get() = appId.startsWith("aliced")
    val isSmartSpeaker: Boolean
        get() = isQuasar || isMiniSpeaker || isStationMini
    val isYaSmartDevice: Boolean
        get() = isSmartSpeaker || isTvDevice || isCentaur || isLegatus || isSdgTaxi
    val isTvDevice: Boolean
        get() = appId == "com.yandex.tv.alice"
    val isLegatus: Boolean
        get() = appId == "legatus"
    val isSdgTaxi: Boolean
        get() = appId == "ru.yandex.sdg.taxi.inhouse"
    val isTestSmartSpeaker: Boolean
        get() = isSmartSpeaker && appId.endsWith("vins_test")
    val isTouch: Boolean
        get() = isAndroid || isIOS || isTestClient
    val isDesktop: Boolean
        get() = isYaStroka || isYaBrowserDesktop
    val isSearchAppTest: Boolean
        get() = "ru.yandex.mobile.inhouse" == appId || "ru.yandex.mobile.dev" == appId || appId.startsWith("ru.yandex.searchplugin.") || isWeatherPluginTest
    val isSearchAppProd: Boolean
        get() = "ru.yandex.mobile" == appId || "ru.yandex.searchplugin" == appId ||
            isWeatherPluginProd
    val isCentaur: Boolean
        get() = "ru.yandex.centaur" == appId

    // absorbed client: DIALOG-5251
    // assume that all test clients are "Search app"
    val isSearchApp: Boolean
        get() = isSearchAppProd ||
            isSearchAppTest ||
            isWeatherPlugin ||  // absorbed client: DIALOG-5251
            isTestClient ||  // assume that all test clients are "Search app"
            isSampleApp

    fun supportsDivCards(): Boolean {
        return if (isYaAuto) {
            false // coz yandex auto is android without support div cards
        } else isSearchApp &&
            (isAndroidAppOfVersionOrNewer(6, 60, 0, 0) ||
                isIOS && (isOsOfVersionOrNewer(11, 0, 0, 0) || isIosOfVersionOrNewer(350))) ||
            isYaBrowser && (isAndroidAppOfVersionOrNewer(17, 10, 1, 291) || isIOS) ||
            isYaBrowserDesktop ||
            isYaStroka && isAppOfVersionOrNewer(1, 12, 0, 0) ||
            isSampleApp ||
            isYaLeftScreen ||
            isYaLauncher ||
            isYaMessenger
    }

    private fun isAndroidAppOfVersionOrNewer(major: Int, minor: Int, patch: Int, build: Int): Boolean {
        return isAndroid && isAppOfVersionOrNewer(major, minor, patch, build)
    }

    private fun isAppOfVersionOrNewer(major: Int, minor: Int, patch: Int, build: Int): Boolean {
        return (appSemVersion != null
            && appSemVersion >= Version(intArrayOf(major, minor, patch, build)))
    }

    private fun isOsOfVersionOrNewer(major: Int, minor: Int, patch: Int, build: Int): Boolean {
        return (osSemVersion != null
            && osSemVersion >= Version(intArrayOf(major, minor, patch, build)))
    }

    private fun isIosOfVersionOrNewer(other: Int): Boolean {
        return (this.iOsVersion != null
            && this.iOsVersion >= other)
    }

    val isWeatherPluginTest: Boolean
        get() = appId.startsWith("ru.yandex.weatherplugin.")
    val isWeatherPluginProd: Boolean
        get() = "ru.yandex.weatherplugin" == appId
    val isWeatherPlugin: Boolean
        get() = isWeatherPluginTest || isWeatherPluginProd
    val isAliceKit: Boolean
        get() = appId.startsWith("ru.yandex.mobile.alice") ||
            appId.startsWith("com.yandex.alicekit")
    val isAliceKitDemo: Boolean
        get() = appId.startsWith("com.yandex.alicekit.demo")
    val isSampleApp: Boolean
        get() = "com.yandex.dialog_assistant.sample" == appId || "ru.yandex.mobile.search.dialog_assistant_sample" == appId ||
            isYaBrowserIpadTest ||
            isAliceKit
    val isYaStroka: Boolean
        get() = "winsearchbar" == appId
    val isYaBrowserIpadTest: Boolean
        get() = "ru.yandex.mobile.search.ipad.inhouse" == appId || "ru.yandex.mobile.search.ipad.dev" == appId || "ru.yandex.mobile.search.ipad.test" == appId
    val isYaBrowserIpadProd: Boolean
        get() = "ru.yandex.mobile.search.ipad" == appId
    val isYaBrowserIpad: Boolean
        get() = isYaBrowserIpadTest ||
            isYaBrowserIpadProd
    val isYaBrowserTest: Boolean
        get() = isYaBrowserTestDesktop || isYaBrowserTestMobile
    val isYaBrowserTestDesktop: Boolean
        get() = "yabro.beta" == appId || "yabro.broteam" == appId || "yabro.canary" == appId || "yabro.dev" == appId

    // iOS
    val isYaBrowserTestMobile: Boolean
        get() = // iOS
            "ru.yandex.mobile.search.inhouse" == appId || "ru.yandex.mobile.search.dev" == appId || "ru.yandex.mobile.search.test" == appId || "com.yandex.browser.beta" == appId || "com.yandex.browser.alpha" == appId || "com.yandex.browser.inhouse" == appId || "com.yandex.browser.dev" == appId || "com.yandex.browser.canary" == appId || "com.yandex.browser.broteam" == appId
    val isYaBrowserCanary: Boolean
        get() = isYaBrowserCanaryDesktop || isYaBrowserCanaryMobile
    val isYaBrowserCanaryDesktop: Boolean
        get() = "yabro.canary" == appId
    val isYaBrowserCanaryMobile: Boolean
        get() = "com.yandex.browser.canary" == appId
    val isYaBrowserProd: Boolean
        get() = isYaBrowserProdDesktop || isYaBrowserProdMobile
    val isYaBrowserProdDesktop: Boolean
        get() = "yabro" == appId
    val isYaBrowserProdMobile: Boolean
        get() = "ru.yandex.mobile.search" == appId || "com.yandex.browser" == appId
    val isYaBrowser: Boolean
        get() = isYaBrowserTest ||
            isYaBrowserProd
    val isYaBrowserDesktop: Boolean
        get() = isYaBrowserTestDesktop ||
            isYaBrowserCanaryDesktop ||
            isYaBrowserProdDesktop

    val isYaBrowserMobile: Boolean
        get() = isYaBrowserIpad ||
            isYaBrowserTestMobile ||
            isYaBrowserCanaryMobile ||
            isYaBrowserProdMobile

    // Android
    val isYaLeftScreen: Boolean
        get() = appId.startsWith("ru.yandex.leftscreen") // Android

    // Android
    val isYaLauncher: Boolean
        get() = appId.startsWith("com.yandex.launcher") // Android
    val isNavigatorOrMaps: Boolean
        get() = appId.startsWith("ru.yandex.mobile.navigator") ||
            appId.startsWith("ru.yandex.yandexnavi") ||
            appId.startsWith("ru.yandex.yandexmaps") ||
            appId.startsWith("ru.yandex.traffic")
    val isNavigatorBeta: Boolean
        get() = "ru.yandex.mobile.navigator.inhouse" == appId || "ru.yandex.mobile.navigator.sandbox" == appId || "ru.yandex.yandexnavi.inhouse" == appId || "ru.yandex.yandexnavi.sandbox" == appId
    val isYaAuto: Boolean
        get() = appId.startsWith("yandex.auto")
    val isClientWithNavigator: Boolean
        get() = isNavigatorOrMaps ||
            isYaAuto
    val isPpBeta: Boolean
        get() = "ru.yandex.searchplugin.beta" == appId || "ru.yandex.searchplugin.dev" == appId
    val isPpNightly: Boolean
        get() = "ru.yandex.searchplugin.nightly" == appId
    val isElariWatch: Boolean
        get() = appId.startsWith("ru.yandex.iosdk.elariwatch")
    val isYaMessenger: Boolean
        get() = "ru.yandex.messenger" == appId
    val isYaMusicTest: Boolean
        get() = appId.startsWith("ns.mobile.music")
    val isYaMusicProd: Boolean
        get() = appId.startsWith("ru.yandex.mobile.music") ||
            appId.startsWith("ru.yandex.music")
    val isYaMusic: Boolean
        get() = isYaMusicTest || isYaMusicProd
    val isDevConsole: Boolean
        get() = uuid.startsWith("dev-console")
    val isFloyd: Boolean
        get() = uuid.startsWith("floyd-uuid")
    val isStationMini: Boolean
        get() = "Yandex" == deviceManufacturer && "yandexmini" == deviceModel

    val surface: Surface?
        get() = APP_ID_TO_SURFACE_MAPPING[appId]

    fun hasAudioPlayer(): Boolean {
        return interfaces.hasAudioClient && !(isYaMessenger || isYaBrowser)
    }

    private fun createSemVersion(version: String): Version? {
        return try {
            Version(version)
        } catch (e: Exception) {
            null
        }
    }

    val isSupportModrovia: Boolean
        get() = interfaces.hasMordoviaWebView

    class Version : Comparable<Version> {

        private val versionNumbers: IntArray

        internal constructor(versionNumbers: IntArray) {
            this.versionNumbers = versionNumbers
        }

        internal constructor(versionNumbers: String) {
            this.versionNumbers = versionNumbers.split("-")
                .first()
                .split(".")
                .map { it.toInt() }
                .toIntArray()
        }

        override fun compareTo(other: Version): Int {
            val maxLength = max(versionNumbers.size, other.versionNumbers.size)
            for (i in 0 until maxLength) {
                val left = if (i < versionNumbers.size) versionNumbers[i] else 0
                val right = if (i < other.versionNumbers.size) other.versionNumbers[i] else 0
                if (left != right) {
                    return if (left < right) -1 else 1
                }
            }
            return 0
        }
    }

    data class ClientInfoBuilder(
        var appId: String? = null,
        var appVersion: String? = null,
        var platform: String? = null,
        var osVersion: String? = null,
        var uuid: String? = null,
        var deviceId: String? = null,
        var lang: String? = null,
        var timezone: String? = null,
        var deviceModel: String? = null,
        var deviceManufacturer: String? = null,
        var interfaces: Interfaces = Interfaces()
    ) {
        fun build() = ClientInfo(
            appId = appId,
            appVersion = appVersion,
            platform = platform,
            osVersion = osVersion,
            uuid = uuid,
            deviceId = deviceId,
            lang = lang,
            timezone = timezone,
            deviceModel = deviceModel,
            deviceManufacturer = deviceManufacturer,
            interfaces = interfaces,
        )

        fun appId(v: String) = this.apply { appId = v }
        fun appVersion(v: String) = this.apply { appVersion = v }
        fun platform(v: String) = this.apply { platform = v }
        fun osVersion(v: String) = this.apply { osVersion = v }
        fun uuid(v: String) = this.apply { uuid = v }
        fun deviceId(v: String) = this.apply { deviceId = v }
        fun deviceId(v: Optional<String>) = this.apply { deviceId = v.orElse(null) }
        fun lang(v: String) = this.apply { lang = v }
        fun timezone(v: String) = this.apply { timezone = v }
        fun deviceModel(v: String) = this.apply { deviceModel = v }
        fun deviceManufacturer(v: String) = this.apply { deviceManufacturer = v }
        fun interfaces(v: Interfaces) = this.apply { interfaces = v }
    }

    companion object {
        @JvmStatic
        fun builder() = ClientInfoBuilder()
    }
}

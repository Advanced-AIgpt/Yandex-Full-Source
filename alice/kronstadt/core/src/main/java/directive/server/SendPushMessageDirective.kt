package ru.yandex.alice.kronstadt.core.directive.server

import java.net.URI
import java.time.Duration

data class SendPushMessageDirective @JvmOverloads constructor(
    val title: String,
    val body: String,
    val link: URI,
    val pushId: String,
    val pushTag: String,
    val throttlePolicy: String,
    val appTypes: List<AppType> = listOf(),
    val cardTitle: String? = null,
    val cardButtonText: String? = null,
    val ttl: Duration = Duration.ofHours(2),
) : ServerDirective {

    enum class AppType {
        UNDEFINED, SEARCH_APP, SMART_SPEAKER, MOBILE_BROWSER, DESKTOP_BROWSER
    }

    companion object {
        const val ALICE_DEFAULT_DEVICE_ID = "alice_default_device_id"
    }
}

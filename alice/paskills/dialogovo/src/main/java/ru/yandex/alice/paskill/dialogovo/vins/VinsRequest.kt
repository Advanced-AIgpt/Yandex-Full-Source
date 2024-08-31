package ru.yandex.alice.paskill.dialogovo.vins

import com.fasterxml.jackson.annotation.JsonProperty
import ru.yandex.alice.kronstadt.core.domain.LocationInfo

data class VinsRequest(
    val meta: VinsMeta,
    val form: VinsForm,
    val action: VinsAction? = null,
    val location: LocationInfo? = null
) {

    data class VinsAction(val name: String, val data: Any) {

        companion object {
            const val ACCOUNT_LINKING_COMPLETE_EVENT_NAME = "external_skill__account_linking_complete"
        }
    }

    data class VinsMeta(
        val epoch: Long,
        val tz: String,
        @JsonProperty("client_id") val clientId: String?,
        val uuid: String,
        val uid: String?,
        // wtf?
        val utterance: String,
        val experiments: Map<String, String>,
    )

    internal data class VinsClientInfo(
        @JsonProperty("app_id") val appId: String,
        @JsonProperty("app_version") val appVersion: String,
        @JsonProperty("device_manufacturer") val deviceManufacturer: String,
        @JsonProperty("device_model") val deviceModel: String,
        @JsonProperty("os_version") val osVersion: String,
        @JsonProperty("platform") val platform: String,
    )
}

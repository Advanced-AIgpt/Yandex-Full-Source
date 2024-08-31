package ru.yandex.alice.kronstadt.core.domain

import com.fasterxml.jackson.annotation.JsonProperty
import ru.yandex.alice.megamind.protos.scenarios.RequestProto

/**
 * Serializable representation of RequestProto.TInterfaces.TInterfaces()
 */
data class Interfaces(
    @JsonProperty("has_server_action") val canServerAction: Boolean = false,
    @JsonProperty("has_synchronized_push") val hasSynchronizedPush: Boolean = false,
    @JsonProperty("has_mordovia_web_view") val hasMordoviaWebView: Boolean = false,
    @JsonProperty("has_audio_client") val hasAudioClient: Boolean = false,
    @JsonProperty("supports_audio_bitrate_192_kpbs") val supportsAudioBitrate192Kbps: Boolean = false,
    @JsonProperty("supports_audio_bitrate_320_kpbs") val supportsAudioBitrate320Kbps: Boolean = false,
    @JsonProperty("supports_show_view") val supportsShowView: Boolean = false,
) {
    companion object {
        @JvmStatic
        val EMPTY: Interfaces = Interfaces()

        fun fromProto(protoInterfaces: RequestProto.TInterfaces): Interfaces {
            return Interfaces(
                protoInterfaces.canServerAction,
                protoInterfaces.hasSynchronizedPush,
                protoInterfaces.hasMordoviaWebView,
                protoInterfaces.hasAudioClient,
                protoInterfaces.supportsAudioBitrate192Kbps,
                protoInterfaces.supportsAudioBitrate320Kbps,
                protoInterfaces.supportsShowView,
            )
        }
    }
}

package ru.yandex.alice.kronstadt.core

import ru.yandex.alice.protos.endpoint.CapabilityProto

data class VideoCallCapability(
    val providerStates: List<ProviderState> = listOf(),
    val incoming: List<ProviderCall> = listOf(),
    val current: ProviderCall? = null,
    val outgoing: ProviderCall? = null,
    val supportedDirectives: List<CapabilityProto.TCapability.EDirectiveType> = listOf(),
) {
    sealed class ProviderState

    data class TelegramState(
        val login: Login,
        val contactSyncProcess: Boolean,
    ): ProviderState() {
        data class Login(
            val userId: String,
            val state: State,
            val fullContactsUploadFinished: Boolean
        ) {
            enum class State { IN_PROGRESS, SUCCESS }
        }
    }

    data class TelegramVideoCallOwnerData(
        val userId: String,
        val callId: String,
    )

    data class ProviderCall(
        val state: State,
        val providerCallData: ProviderCallData,
    ) {
        enum class State { RINGING, ACCEPTED, ESTABLISHED }
    }

    sealed class ProviderCallData

    data class TelegramCallData(
        val callOwnerData: TelegramVideoCallOwnerData,
        val recipient: RecipientData,
       val micMuted: Boolean,
       val videoEnabled: Boolean,
    ): ProviderCallData() {
        data class RecipientData(
            val userId: String,
            val displayName: String,
        )
    }

    companion object {
        val NOOP_VIDEO_CALL_CAPABILITY = VideoCallCapability()
    }
}
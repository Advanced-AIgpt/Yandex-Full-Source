package ru.yandex.alice.kronstadt.scenarios.video_call.utils

import org.apache.logging.log4j.LogManager
import ru.yandex.alice.kronstadt.core.VideoCallCapability
import ru.yandex.alice.kronstadt.core.domain.TELEGRAM_CONTACT_TYPE_PREFIX
import ru.yandex.alice.protos.endpoint.CapabilityProto.TCapability.EDirectiveType

object VideoCallCapabilityExtension {
    private val logger = LogManager.getLogger(VideoCallCapabilityExtension::class.java)

    private fun VideoCallCapability.getTelegramState() =
        providerStates.filterIsInstance<VideoCallCapability.TelegramState>().firstOrNull()

    // ---------- <login state
    fun VideoCallCapability.checkTelegramLogin(): Boolean =
        getTelegramState()?.let { it.login.state == VideoCallCapability.TelegramState.Login.State.SUCCESS } ?: false

    fun VideoCallCapability.getTelegramUserId() =
        if (checkTelegramLogin()) {
            getTelegramState()!!.login.userId
        } else null

    fun VideoCallCapability.getTelegramAccountType(): String? =
        getTelegramUserId()?.let { "${TELEGRAM_CONTACT_TYPE_PREFIX}_$it" }
    // ---------- login state>

    // ---------- <contacts state
    fun VideoCallCapability.checkTelegramContactsUpload(): Boolean =
        getTelegramState()?.login?.fullContactsUploadFinished ?: false
    // ---------- contacts state>

    // ---------- <calls state
    // -------------------- <active & outgoing call
    fun VideoCallCapability.hasActiveCall() = current != null

    fun VideoCallCapability.getActiveCall(): VideoCallCapability.TelegramCallData? {
        return when (val providerCallData = current?.providerCallData) {
            is VideoCallCapability.TelegramCallData -> providerCallData
            else -> null
        }
    }
    fun VideoCallCapability.hasOutgoingCall() = outgoing != null

    fun VideoCallCapability.getOutgoingCall(): VideoCallCapability.TelegramCallData? {
        return when(val providerCallData = outgoing?.providerCallData) {
            is VideoCallCapability.TelegramCallData -> providerCallData
            else -> null
        }
    }

    fun VideoCallCapability.hasActiveOrOutgoingCall() = hasActiveCall() || hasOutgoingCall()
    fun VideoCallCapability.getActiveOrOutgoingCall() = getActiveCall() ?: getOutgoingCall()

    fun VideoCallCapability.isMicMuted() = getActiveOrOutgoingCall()?.micMuted ?: false
    fun VideoCallCapability.getVideoEnabled() = getActiveOrOutgoingCall()?.videoEnabled ?: false
    // -------------------- active & outgoing call>

    // -------------------- <incoming call
    private fun VideoCallCapability.getIncomingCall() = incoming.firstOrNull {
        it.state == VideoCallCapability.ProviderCall.State.RINGING && it.providerCallData is VideoCallCapability.TelegramCallData
    }

    fun VideoCallCapability.hasIncomingCall() = getIncomingCall() != null

    fun VideoCallCapability.getIncomingTelegramCallData(): VideoCallCapability.TelegramCallData? {
        return when (val providerCallData = getIncomingCall()?.providerCallData) {
            is VideoCallCapability.TelegramCallData -> providerCallData
            else -> null
        }
    }
    // -------------------- incoming call>
    // ---------- calls state>

    // ---------- <directives supports
    fun VideoCallCapability.checkSupportsStartVideoCallLogin() =
        supportedDirectives.contains(EDirectiveType.StartVideoCallLoginDirectiveType)
            .also { supports -> if(!supports) logger.warn("Device doesn't support video call login") }

    fun VideoCallCapability.checkSupportsStartVideoCall() =
        supportedDirectives.contains(EDirectiveType.StartVideoCallDirectiveType)
            .also { supports -> if(!supports) logger.warn("Device doesn't support outgoing video call") }

    fun VideoCallCapability.checkSupportsAcceptVideoCall() =
        supportedDirectives.contains(EDirectiveType.AcceptVideoCallDirectiveType)
            .also { supports -> if(!supports) logger.warn("Device doesn't support accepting video call") }

    fun VideoCallCapability.checkSupportsDiscardVideoCall() =
        supportedDirectives.contains(EDirectiveType.DiscardVideoCallDirectiveType)
            .also { supports -> if(!supports) logger.warn("Device doesn't support discarding video call") }

    fun VideoCallCapability.checkSupportsMuteMic() =
        supportedDirectives.contains(EDirectiveType.VideoCallMuteMicDirectiveType)
            .also { supports -> if(!supports) logger.warn("Device doesn't support mute mic") }

    fun VideoCallCapability.checkSupportsUnmuteMic() =
        supportedDirectives.contains(EDirectiveType.VideoCallUnmuteMicDirectiveType)
            .also { supports -> if(!supports) logger.warn("Device doesn't support unmute mic") }
    // ---------- directives supports>
}

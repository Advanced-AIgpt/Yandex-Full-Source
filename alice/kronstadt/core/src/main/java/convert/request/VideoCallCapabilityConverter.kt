package ru.yandex.alice.kronstadt.core.convert.request

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.VideoCallCapability
import ru.yandex.alice.protos.endpoint.CapabilityProto.TVideoCallCapability
import ru.yandex.alice.protos.endpoint.CapabilityProto.TVideoCallCapability.TProviderCall
import ru.yandex.alice.protos.endpoint.CapabilityProto.TVideoCallCapability.TTelegramProviderState

@Component
open class VideoCallCapabilityConverter : FromProtoConverter<TVideoCallCapability, VideoCallCapability> {
   
    override fun convert(src: TVideoCallCapability): VideoCallCapability {
        return VideoCallCapability(
            providerStates = src.state.providerStatesList.mapNotNull { providerState ->
                when {
                    providerState.hasTelegramProviderState() -> {
                        val tTelegramState = providerState.telegramProviderState
                        VideoCallCapability.TelegramState(
                            login = VideoCallCapability.TelegramState.Login(
                                userId = tTelegramState.login.userId,
                                state = convertTelegramLoginState(tTelegramState.login.state),
                                fullContactsUploadFinished = tTelegramState.login.fullContactsUploadFinished.or(
                                    false
                                )
                            ),
                            contactSyncProcess = tTelegramState.hasContactSync()
                        )
                    }
                    else -> null
                }
            },
            incoming = if (src.state.incomingCount != 0) {
                src.state.incomingList.mapNotNull { incoming -> convertProviderCall(incoming) }
            } else { listOf() },
            current = if (src.state.hasCurrent()) {
                convertProviderCall(src.state.current)
            } else { null },
            outgoing = if (src.state.hasOutgoing()) {
                convertProviderCall(src.state.outgoing)
            } else { null },
            supportedDirectives = src.meta.supportedDirectivesList,
        )
    }

    private fun convertTelegramLoginState(telegramLoginState: TTelegramProviderState.TLogin.EState) =
        when (telegramLoginState) {
            TTelegramProviderState.TLogin.EState.InProgress -> VideoCallCapability.TelegramState.Login.State.IN_PROGRESS
            TTelegramProviderState.TLogin.EState.Success -> VideoCallCapability.TelegramState.Login.State.SUCCESS
            else -> throw IllegalStateException("Unknown telegram login state")
        }

    private fun convertProviderCall(providerCall: TProviderCall): VideoCallCapability.ProviderCall {
        return VideoCallCapability.ProviderCall(
            state = convertProviderCallState(providerCall.state),
            providerCallData = when {
                providerCall.hasTelegramCallData() -> {
                    val tTelegramCallData = providerCall.telegramCallData
                    VideoCallCapability.TelegramCallData(
                        callOwnerData = VideoCallCapability.TelegramVideoCallOwnerData(
                            userId = tTelegramCallData.callOwnerData.userId,
                            callId = tTelegramCallData.callOwnerData.callId,
                        ),
                        recipient = VideoCallCapability.TelegramCallData.RecipientData(
                            userId = tTelegramCallData.recipient.userId,
                            displayName = tTelegramCallData.recipient.displayName,
                        ),
                        micMuted = tTelegramCallData.micMuted,
                        videoEnabled = tTelegramCallData.videoEnabled,
                    )
                }
                else -> throw IllegalStateException("Unknown provider call data")
            }
        )
    }

    private fun convertProviderCallState(providerCallState: TProviderCall.EState) =
        when (providerCallState) {
            TProviderCall.EState.Accepted -> VideoCallCapability.ProviderCall.State.ACCEPTED
            TProviderCall.EState.Ringing -> VideoCallCapability.ProviderCall.State.RINGING
            TProviderCall.EState.Established -> VideoCallCapability.ProviderCall.State.ESTABLISHED
            else -> throw IllegalStateException("Unknown provider call state")
        }
}
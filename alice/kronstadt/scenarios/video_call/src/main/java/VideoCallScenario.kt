package ru.yandex.alice.kronstadt.scenarios.video_call

import org.apache.logging.log4j.LogManager
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.ScenarioMeta
import ru.yandex.alice.kronstadt.core.VideoCallCapability
import ru.yandex.alice.kronstadt.core.convert.response.LayoutConverter
import ru.yandex.alice.kronstadt.core.scenario.AbstractNoStateScenario
import ru.yandex.alice.kronstadt.core.scenario.SelectedScene
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticFrame
import ru.yandex.alice.kronstadt.scenarios.video_call.domain.ProviderContactData
import ru.yandex.alice.kronstadt.scenarios.video_call.domain.converter.ProviderContactDataConverter
import ru.yandex.alice.kronstadt.scenarios.video_call.scenes.AcceptIncomingCallScene
import ru.yandex.alice.kronstadt.scenarios.video_call.scenes.ActivateVideoCallScene
import ru.yandex.alice.kronstadt.scenarios.video_call.scenes.CallFailedCallbackScene
import ru.yandex.alice.kronstadt.scenarios.video_call.scenes.DiscardIncomingCallScene
import ru.yandex.alice.kronstadt.scenarios.video_call.scenes.IncomingVideoCallScene
import ru.yandex.alice.kronstadt.scenarios.video_call.scenes.LoginContactsNotUploaded
import ru.yandex.alice.kronstadt.scenarios.video_call.scenes.LoginFailedCallbackScene
import ru.yandex.alice.kronstadt.scenarios.video_call.scenes.NoMatchingContactsScene
import ru.yandex.alice.kronstadt.scenarios.video_call.scenes.OutgoingAcceptedScene
import ru.yandex.alice.kronstadt.scenarios.video_call.scenes.SelectContactScene
import ru.yandex.alice.kronstadt.scenarios.video_call.scenes.StartLoginScene
import ru.yandex.alice.kronstadt.scenarios.video_call.scenes.VideoCallMainScreenScene
import ru.yandex.alice.kronstadt.scenarios.video_call.scenes.VideoCallStateUpdateScene
import ru.yandex.alice.kronstadt.scenarios.video_call.scenes.*
import ru.yandex.alice.kronstadt.scenarios.video_call.utils.MatchedContactsBuilder
import ru.yandex.alice.kronstadt.scenarios.video_call.utils.VideoCallCapabilityExtension.checkSupportsAcceptVideoCall
import ru.yandex.alice.kronstadt.scenarios.video_call.utils.VideoCallCapabilityExtension.checkSupportsDiscardVideoCall
import ru.yandex.alice.kronstadt.scenarios.video_call.utils.VideoCallCapabilityExtension.checkSupportsMuteMic
import ru.yandex.alice.kronstadt.scenarios.video_call.utils.VideoCallCapabilityExtension.checkSupportsStartVideoCall
import ru.yandex.alice.kronstadt.scenarios.video_call.utils.VideoCallCapabilityExtension.checkSupportsStartVideoCallLogin
import ru.yandex.alice.kronstadt.scenarios.video_call.utils.VideoCallCapabilityExtension.checkSupportsUnmuteMic
import ru.yandex.alice.kronstadt.scenarios.video_call.utils.VideoCallCapabilityExtension.checkTelegramContactsUpload
import ru.yandex.alice.kronstadt.scenarios.video_call.utils.VideoCallCapabilityExtension.checkTelegramLogin
import ru.yandex.alice.kronstadt.scenarios.video_call.utils.VideoCallCapabilityExtension.getTelegramUserId
import ru.yandex.alice.kronstadt.scenarios.video_call.utils.VideoCallCapabilityExtension.hasActiveCall
import ru.yandex.alice.kronstadt.scenarios.video_call.utils.VideoCallCapabilityExtension.hasActiveOrOutgoingCall
import ru.yandex.alice.kronstadt.scenarios.video_call.utils.VideoCallCapabilityExtension.hasIncomingCall
import ru.yandex.alice.kronstadt.scenarios.video_call.utils.VideoCallCapabilityExtension.hasOutgoingCall
import ru.yandex.alice.kronstadt.scenarios.video_call.utils.VideoCallCapabilityExtension.isMicMuted
import ru.yandex.alice.kronstadt.scenarios.video_call.utils.matchContactByUserIdAndContactId
import ru.yandex.alice.protos.endpoint.CapabilityProto
import ru.yandex.web.apphost.api.request.ApphostResponseBuilder

val VIDEO_CALL = ScenarioMeta(
    name = "video_call",
    megamindName = "VideoCall",
    productScenarioName = "video_call",
    useDivRenderer = true)

const val IGNORE_CAPABILITY_FOR_UE2E = "video_call_ignore_capability_for_ue2e"

@Component
class VideoCallScenario(
    private val matchedContactsBuilder: MatchedContactsBuilder,
    private val providerContactDataConverter: ProviderContactDataConverter
) : AbstractNoStateScenario(VIDEO_CALL) {

    private val logger = LogManager.getLogger(LayoutConverter::class.java)

    override fun prepareSelectScene(
        request: MegaMindRequest<Any>,
        responseBuilder: ApphostResponseBuilder
    ) {
        //noop prepare
    }

    override fun selectScene(request: MegaMindRequest<Any>): SelectedScene.Running<Any, *>? =
        request.handle {
            onCondition({ videoCallCapability != null || hasExperiment(IGNORE_CAPABILITY_FOR_UE2E) }) {
                with(videoCallCapability ?: VideoCallCapability.NOOP_VIDEO_CALL_CAPABILITY) {
                    // ---------- call controlling
                    onFrame(MESSENGER_ACCEPT_INCOMING_CALL_FRAME) {
                        if (hasIncomingCall() && checkSupportsAcceptVideoCall()) {
                                scene<AcceptIncomingCallScene>()
                        } else null
                    }
                    onFrame(MESSENGER_DISCARD_INCOMING_CALL_FRAME) {
                        if (hasIncomingCall() && checkSupportsDiscardVideoCall()) {
                                scene<DiscardIncomingCallScene>()
                        } else null
                    }
                    onFrame(MESSENGER_HANGUP_CALL_FRAME) {
                        onCondition({ hasActiveCall() || hasOutgoingCall() }) {
                            scene<HangupCallScene>()
                        }
                    }
                    onFrame(VIDEO_CALL_INCOMING_FRAME) { frame ->
                        with(frame.typedSemanticFrame!!.videoCallIncomingSemanticFrame!!) {
                            sceneWithArgs(
                                IncomingVideoCallScene::class,
                                IncomingVideoCallScene.Args(
                                    callId = callId.stringValue,
                                    userId = userId.stringValue,
                                    caller = request.contactsList?.matchContactByUserIdAndContactId(
                                        userId.stringValue,
                                        caller.contactData.telegramContactData.userId
                                    )?.let { ProviderContactData(it) }
                                )
                            )
                        }
                    }
                    onAnyFrame(MESSENGER_CALL_TO_FRAME, PHONE_CALL_TO_FRAME, VIDEO_CALL_TO_FRAME) {
                        if (hasExperiment(IGNORE_CAPABILITY_FOR_UE2E) || checkSupportsStartVideoCall()) {
                                onCondition({ !hasExperiment(IGNORE_CAPABILITY_FOR_UE2E) && !checkTelegramLogin() }) {
                                    if (checkSupportsStartVideoCallLogin()) {
                                        scene<StartLoginScene>()
                                    } else null
                                }
                                onCondition({ !hasExperiment(IGNORE_CAPABILITY_FOR_UE2E) && !checkTelegramContactsUpload() }) {
                                    scene<LoginContactsNotUploaded>()
                                }
                                onFrame(VIDEO_CALL_TO_FRAME) { frame ->
                                    onCondition({ frame.typedSemanticFrame?.videoCallToSemanticFrame != null }) {
                                        val contactData = providerContactDataConverter.convert(
                                            frame.typedSemanticFrame!!.videoCallToSemanticFrame.fixedContact.contactData)
                                        sceneWithArgs(
                                            ActivateVideoCallScene::class,
                                            ActivateVideoCallScene.Args(
                                                getUserId(request),
                                                contactData,
                                                request.contactsList!!.matchContactByUserIdAndContactId(
                                                    getUserId(request),
                                                    contactData.userId)!!,
                                                frame.getBootSlotValue(VIDEO_ENABLED_SLOT)?: false,
                                            )
                                        )
                                    }
                                }
                                onFrame(PHONE_CALL_TO_FRAME) { frame ->
                                    val contactBookItemNamesSlot = frame.getFirstSlot(CONTACT_BOOK_ITEM_NAME)
                                    onCondition({ contactsList != null && contactBookItemNamesSlot != null }) {
                                        val matchedContacts =
                                            matchedContactsBuilder.buildMatchedContacts(contactBookItemNamesSlot!!, request)
                                        logger.debug("Matched contacts: $matchedContacts")
                                        onCondition({ matchedContacts.size == 1 }) {
                                            sceneWithArgs(
                                                ActivateVideoCallScene::class,
                                                ActivateVideoCallScene.Args(
                                                    getUserId(request),
                                                    ProviderContactData(matchedContacts.first()),
                                                    matchedContacts.first(),
                                                )
                                            )
                                        }
                                        onCondition({ matchedContacts.size > 1 }) {
                                            sceneWithArgs(
                                                SelectContactScene::class,
                                                SelectContactScene.Args(matchedContacts)
                                            )
                                        }
                                    }
                                }
                                scene<NoMatchingContactsScene>()
                        } else null
                    }
                    // ---------- mic & video controlling
                    onFrame(VIDEO_CALL_MUTE_MIC) {
                        onCondition({ hasActiveOrOutgoingCall() && !isMicMuted() && checkSupportsMuteMic() }) {
                            scene<VideoCallMuteMicScene>()
                        }
                    }
                    onFrame(VIDEO_CALL_UNMUTE_MIC) {
                        onCondition({ hasActiveOrOutgoingCall() && isMicMuted() && checkSupportsUnmuteMic() }) {
                            scene<VideoCallUnmuteMicScene>()
                        }
                    }

                    // ---------- widget data
                    onAnyFrame(CENTAUR_COLLECT_MAIN_SCREEN, CENTAUR_COLLECT_WIDGET_GALLERY) {
                        scene<VideoCallMainScreenScene>()
                    }

                    // ---------- contacts - postponed it
                    /*
                    onFrame(PHONE_CALL_OPEN_ADDRESS_BOOK) {
                        scene<OpenContactBookScene>()
                    }
                    onFrame(SET_FAVORITES) { frame ->
                        onCondition({ frame.typedSemanticFrame?.videoCallSetFavoritesSemanticFrame != null }) {
                            scene<SetFavoritesScene>()
                        }
                    }
                     */

                    // ---------- capability state updates
                    onFrame(ENDPOINT_STATE_UPDATES_FRAME) { frame ->
                        onCondition({ hasVideoCallStateUpdate(frame, request) }) {
                            scene<VideoCallStateUpdateScene>()
                        }
                    }
                    // ---------- callbacks
                    onFrame(VIDEO_CALL_OUTGOING_ACCEPTED_FRAME) { frame ->
                        with(frame.typedSemanticFrame!!.videoCallOutgoingAcceptedSemanticFrame) {
                            if (hasCallId()) {
                                val contactData = providerContactDataConverter.convert(contact.contactData)
                                sceneWithArgs(
                                    OutgoingAcceptedScene::class,
                                    OutgoingAcceptedScene.Args(
                                        callId = callId.stringValue,
                                        userId = this@with.userId.stringValue,
                                        recipient = contactData,
                                    )
                                )
                            } else null
                        }
                    }
                    onFrame(VIDEO_CALL_LOGIN_FAILED_FRAME) {
                        scene<LoginFailedCallbackScene>()
                    }
                    onAnyFrame(VIDEO_CALL_OUTGOING_FAILED_FRAME, VIDEO_CALL_INCOMING_ACCEPT_FAILED_FRAME) {
                        scene<CallFailedCallbackScene>()
                    }
                }
            }
        }

    private fun getUserId(request: MegaMindRequest<Any>) = if (request.hasExperiment(IGNORE_CAPABILITY_FOR_UE2E)) "ue2e"
        else request.videoCallCapability!!.getTelegramUserId()!!

    private fun hasVideoCallStateUpdate(frame: SemanticFrame, request: MegaMindRequest<Any>) =
        frame.typedSemanticFrame != null && frame.typedSemanticFrame!!.hasEndpointStateUpdatesSemanticFrame()
            && frame.typedSemanticFrame!!.endpointStateUpdatesSemanticFrame.request.requestValue.endpointUpdatesList
            ?.firstOrNull { it.id == request.clientInfo.deviceId }
            ?.capabilitiesList?.any { capability -> capability.`is`(CapabilityProto.TVideoCallCapability::class.java) }
            ?: false
}

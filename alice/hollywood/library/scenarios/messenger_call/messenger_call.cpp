#include "messenger_call.h"

#include "phone_call.h"

#include <alice/hollywood/library/environment_state/endpoint.h>
#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/registry/registry.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/response/push.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/resources/geobase.h>
#include <alice/hollywood/library/s3_animations/s3_animations.h>
#include <alice/hollywood/library/scenarios/messenger_call/proto/call_payload.pb.h>

#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/library/experiments/flags.h>
#include <alice/library/proto/proto.h>
#include <alice/library/url_builder/url_builder.h>

#include <alice/megamind/protos/common/device_state.pb.h>
#include <alice/megamind/protos/common/iot.pb.h>

#include <alice/protos/data/channel/channel.pb.h>
#include <alice/protos/data/device/info.pb.h>
#include <alice/protos/data/location/group.pb.h>
#include <alice/protos/data/location/room.pb.h>
#include <alice/protos/endpoint/capability.pb.h>
#include <alice/protos/endpoint/capabilities/audio_file_player/capability.pb.h>
#include <alice/protos/endpoint/endpoint.pb.h>

#include <library/cpp/resource/resource.h>
#include <library/cpp/string_utils/quote/quote.h>

#include <util/generic/maybe.h>
#include <util/generic/hash_set.h>
#include <util/generic/variant.h>
#include <util/string/cast.h>
#include <util/string/join.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood {

namespace {

constexpr TStringBuf ACCEPT_CALL_FRAME = "alice.messenger_call.accept_incoming_call";
constexpr TStringBuf DECLINE_CALL_FRAME = "alice.messenger_call.stop_incoming_call";
constexpr TStringBuf HANGUP_CALL_FRAME = "alice.messenger_call.stop_current_call";
constexpr TStringBuf GET_CALLER_NAME_FRAME = "alice.messenger_call.get_caller_name";
constexpr TStringBuf CALL_TO_OPERATOR_FRAME = "alice.messenger_call.call_to_operator";
constexpr TStringBuf CALL_TO_FRAME = "alice.messenger_call.call_to";
constexpr TStringBuf CALL_TO_NANNY_FRAME = "alice.messenger_call.call_to_nanny";
constexpr TStringBuf ASK_RECEPIENT_FRAME = "alice.messenger_call.call_target";
constexpr TStringBuf DEVICE_CALL_SHORTCUT_FRAME = "alice.messenger_call.device_call_shortcut";

constexpr TStringBuf PHONEBOOK_FILENAME = "emergency_phones.json";
constexpr TStringBuf SOS_SERVICE_ID = "sos";

constexpr TStringBuf EMERGENCY_CALL_INTENT = "emergency_call";
constexpr TStringBuf SPEAKER_CALL_INTENT = "speaker_call";

constexpr TStringBuf DEVICE_SHORTCUT_INTENT = "device_shortcut";
constexpr TStringBuf DEVICE_SHORTCUT_URL = "opensettings://?screen=quasar";

constexpr TStringBuf NANNY_CALL_ACCEPTED_ANIMATION_PATH = "animations/messenger_call/nanny_call_accepted";

const TString DEVICE_CALL_PUSH_TITLE = "Выбрать устройство для звонка";
const TString DEVICE_CALL_PUSH_TEXT = "Задайте названия устройствам или комнатам";
const TString DEVICE_CALL_PUSH_URL = "opensettings://?screen=quasar";
const TString DEVICE_CALL_PUSH_TAG = "alice.device_to_device_call";
const TString DEVICE_CALL_PUSH_POLICY = "unlimited_policy";

const THashSet<TStringBuf> EMERGENCY_SERVICES = {
    TStringBuf("ambulance"),
    TStringBuf("fire_department"),
    TStringBuf("police"),
    TStringBuf("sos")
};

const TVector<TStringBuf> CALLABLE_PLATFORMS = {
    TStringBuf("jbl_link"),
    TStringBuf("yandexmicro"),
    TStringBuf("yandexmidi"),
    TStringBuf("yandexmini"),
    TStringBuf("yandexmini_2"),
    TStringBuf("yandexstation"),
    TStringBuf("yandexstation_2"),
};

const TVector<TStringBuf> STATIONS = {
    TStringBuf("yandexmicro"),
    TStringBuf("yandexmidi"),
    TStringBuf("yandexmini"),
    TStringBuf("yandexmini_2"),
    TStringBuf("yandexstation"),
    TStringBuf("yandexstation_2"),
};

bool isKnownEmergencyService(const TString& s) {
    return EMERGENCY_SERVICES.contains(s);
}

struct TCallState {
    enum class EState {
        IncomingCallRinging = 0,
        CallEstablished = 1,
    };

    EState State;
    TString CallGuid;
    TString PeerName;
    bool MicsMuted;
};

TMaybe<TString> getCallerDeviceId(const TScenarioRunRequestWrapper& request) {
    if (const auto frame = request.Input().FindSemanticFrame(GET_CALLER_NAME_FRAME)) {
        if (const auto slot = TFrame::FromProto(*frame).FindSlot("caller_device_id")) {
            return slot->Value.AsString();;
        }
    }
    return Nothing();
}

TMaybe<TCallState> getCallState(const TScenarioRunRequestWrapper& request) {
    if (!request.Proto().HasBaseRequest() || !request.Proto().GetBaseRequest().HasDeviceState() ||
        !request.Proto().GetBaseRequest().GetDeviceState().HasMessengerCall()) {
        return Nothing();
    }

    const auto& mc = request.Proto().GetBaseRequest().GetDeviceState().GetMessengerCall();

    TCallState state;

    if (mc.HasCurrent()) {
        state.State = TCallState::EState::CallEstablished;
        state.CallGuid = mc.GetCurrent().GetCallId();
        state.PeerName = mc.GetCurrent().GetCallerName();

    } else if (mc.HasIncoming()) {
        state.State = TCallState::EState::IncomingCallRinging;
        state.CallGuid = mc.GetIncoming().GetCallId();
        state.PeerName = mc.GetIncoming().GetCallerName();

    } else {
        return Nothing();
    }

    state.MicsMuted = request.Proto().GetBaseRequest().GetDeviceState().GetMicsMuted();

    return state;
}

class TMessengerCall {
public:
    TMessengerCall(TScenarioHandleContext& ctx)
        : Logger_(ctx.Ctx.Logger())
        , RequestProto_(GetOnlyProtoOrThrow<TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM))
        , Request_(RequestProto_, ctx.ServiceCtx)
        , NlgWrapper_(TNlgWrapper::Create(ctx.Ctx.Nlg(), Request_, ctx.Rng, ctx.UserLang))
        , Builder_(&NlgWrapper_)
        , BodyBuilder_(Builder_.CreateResponseBodyBuilder())
        , UserLocation_(GetUserLocation(Request_))
        , NlgData_(Logger_, Request_)
        , CallState_(getCallState(Request_))
        , IoTUserInfo_(Request_.GetDataSource(NAlice::EDataSourceType::IOT_USER_INFO))
        , Rooms_(GetSmartHomeRooms())
        , Geobase_(ctx.Ctx.GlobalContext().CommonResources().Resource<TGeobaseResource>().GeobaseLookup())
        , Callback_(Request_.Input().GetCallback())
        , OutgoingMessengerCallsEnabled_(Request_.HasExpFlag(NExperiments::EXP_ENABLE_OUTGOING_DEVICE_CALLS))
        , OutgoingOperatorCallsEnabled_(Request_.HasExpFlag(NExperiments::EXP_ENABLE_OUTGOING_OPERATOR_CALLS))
        , DeviceCallShortcutEnabled_(!Request_.HasExpFlag(NExperiments::EXP_HW_DISABLE_DEVICE_CALL_SHORTCUT))
        , State_(Request_.LoadState<TState>())
        , PhoneCall_(Logger_, BodyBuilder_, NlgData_, NlgWrapper_, ctx.Rng, Request_, State_)
        , Endpoint_(GetCurrentEndpoint(Request_))
    {
    }

    std::unique_ptr<TScenarioRunResponse> MakeResponse() && {
        Run();
        BodyBuilder_.SetState(State_);
        return std::move(Builder_).BuildResponse();
    }

private:
    struct TPhoneBook {
        TPhoneBook();
        THashMap<NGeobase::TId, NSc::TValue> EmergencyPhoneBook;
    };

    using TDevice = NAlice::TIoTUserInfo::TDevice;

    enum class EDeviceCallDirectiveType {
        OpenLinkDirective,
        MessengerCallDirective
    };

private:
    void Run() {
        LogInputSources();
        if (Endpoint_) {
            LOG_DEBUG(Logger_) << "Found endpoint: " << JsonFromProto(*Endpoint_);
        }

        if (DeviceCallShortcutEnabled_) {
            if (const auto frame = Request_.Input().FindSemanticFrame(DEVICE_CALL_SHORTCUT_FRAME)) {
                return HandleDeviceShortcut(TFrame::FromProto(*frame));
            }
        }

        if (OutgoingOperatorCallsEnabled_) {
            if (const auto frame = Request_.Input().FindSemanticFrame(CALL_TO_OPERATOR_FRAME)) {
                return HandleOutgoingCallToOperator(TFrame::FromProto(*frame));
            }
        }

        if (const auto frame = Request_.Input().FindSemanticFrame(ACCEPT_CALL_FRAME)) {
            return HandleIncomingCall(TFrame::FromProto(*frame));

        } else if (const auto frame = Request_.Input().FindSemanticFrame(DECLINE_CALL_FRAME); frame && CallState_ && CallState_->State == TCallState::EState::IncomingCallRinging) {
            return HandleIncomingCall(TFrame::FromProto(*frame));

        } else if (const auto frame = Request_.Input().FindSemanticFrame(HANGUP_CALL_FRAME)) {
            return HandleIncomingCall(TFrame::FromProto(*frame));

        } else if (const auto frame = Request_.Input().FindSemanticFrame(GET_CALLER_NAME_FRAME)) {
            return HandleIncomingCall(TFrame::FromProto(*frame));

        } else if (const auto frame = Request_.Input().FindSemanticFrame(CALL_TO_NANNY_FRAME); Request_.HasExpFlag(EXP_ENABLE_CALL_TO_NANNY_ENTRY) && frame) {
            return HandleOutgoingCall(TFrame::FromProto(*frame));

        } else if (const auto frame = Request_.Input().FindSemanticFrame(CALL_TO_FRAME)) {
            return HandleOutgoingCall(TFrame::FromProto(*frame));

        } else if (const auto frame = Request_.Input().FindSemanticFrame(ASK_RECEPIENT_FRAME)) {
            return HandleOutgoingCall(TFrame::FromProto(*frame));

        } else if (!PhoneCall_.TryHandlePhoneCall()) {
            LOG_WARNING(Logger_) << "Returning irrelevant: no valid semantic frame or callback";
            return ReturnIrrelevant();
        }
    }

    void LogInputSources() {
        TVector<TString> frameNames;
        for (const TSemanticFrame& frame : RequestProto_.GetInput().GetSemanticFrames()) {
            frameNames.push_back(frame.GetName());
        }
        LOG_INFO(Logger_) << "Frames in request: " << JoinSeq(", ", frameNames);

        if (const auto* callback = Request_.Input().GetCallback()) {
            LOG_INFO(Logger_) << "Found callback in request: " << callback->GetName();
        }
    }

    void HandleDeviceShortcut(const TFrame&) {
        if (!BodyBuilder_.HasAnalyticsInfoBuilder()) {
            BodyBuilder_.CreateAnalyticsInfoBuilder();
        }
        BodyBuilder_.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::CALL);
        BodyBuilder_.GetAnalyticsInfoBuilder().SetIntentName(TString{DEVICE_SHORTCUT_INTENT});

        TVector<NScenarios::TLayout::TButton> buttons;

        if (!CanOpenQuasarScreen()) {
            NlgData_.AddAttention("unsupported_feature");

        } else {
            buttons.push_back(CreateButton(
                "открыть",
                "open_quasar_settings",
                TString(DEVICE_SHORTCUT_URL),
                true
            ));

            BodyBuilder_.GetAnalyticsInfoBuilder().AddAction(
                "open_quasar_settings",
                "open quasar settings",
                "Открывается страница со списком устройств пользователя"
            );
        }

        BodyBuilder_.AddRenderedTextWithButtonsAndVoice("messenger_call", "device_shortcut_response", buttons, NlgData_);
    }

    void HandleIncomingCall(const TFrame& frame) {
        if (!Request_.BaseRequestProto().GetInterfaces().GetIncomingMessengerCalls()) {
            LOG_WARNING(Logger_) << "Returning irrelevant: client does not support incoming messenger calls";
            return ReturnIrrelevant();
        }

        if (CallState_.Empty()) {
            LOG_WARNING(Logger_) << "Returning irrelevant: empty call state for incoming messenger call";
            return ReturnIrrelevant();
        }

        if (frame.Name() == ACCEPT_CALL_FRAME && CallState_->State == TCallState::EState::IncomingCallRinging) {
            return ReturnAcceptCall();

        } else if (frame.Name() == DECLINE_CALL_FRAME && CallState_->State == TCallState::EState::IncomingCallRinging) {
            TDirective directive;
            directive.MutableMessengerCallDirective()->MutableDeclineIncomingCall()->SetCallGuid(CallState_->CallGuid);
            return ReturnDirective(std::move(directive));

        } else if (frame.Name() == HANGUP_CALL_FRAME) {
            TDirective directive;
            if (CallState_->State == TCallState::EState::IncomingCallRinging) {
                directive.MutableMessengerCallDirective()->MutableDeclineIncomingCall()->SetCallGuid(CallState_->CallGuid);
            } else {
                directive.MutableMessengerCallDirective()->MutableDeclineCurrentCall()->SetCallGuid(CallState_->CallGuid);
            }
            return ReturnDirective(std::move(directive));

        } else if (frame.Name() == GET_CALLER_NAME_FRAME && CallState_->State == TCallState::EState::IncomingCallRinging) {
            bool nannyMode = Request_.HasExpFlag(EXP_FORCE_NANNY_MODE_ON_INCOMING_CALLS);
            if (const auto payload = TryToParseCallPayload(frame)) {
                nannyMode = nannyMode || payload->GetCallToNanny();
            }
            if (nannyMode) {
                return HandleIncomingNannyCall();
            }
            return ReturnCallerName();

        } else {
            LOG_WARNING(Logger_) << "Returning irrelevant: no valid semantic frame for incoming messenger call";
            return ReturnIrrelevant();
        }
    }

    const TMaybe<TCallPayload> TryToParseCallPayload(const TFrame& frame) {
        const auto slot = frame.FindSlot("caller_payload");
        if (!slot) {
            return Nothing();
        }
        const auto& value = slot->Value.AsString();
        if (value.empty()) {
            return Nothing();
        }

        TCallPayload payload;
        if (payload.ParseFromString(Base64Decode(value))) {
            LOG_INFO(Logger_) << "Got TCallPayload: " << JsonFromProto(payload);
            return payload;
        }
        LOG_ERROR(Logger_) << "Failed to parse TCallPayload from: " << value;
        return Nothing();
    }

    TString SerializeToCallPayload(const bool callToNanny) {
        TCallPayload payload;
        payload.SetCallToNanny(callToNanny);
        TString buffer;
        Y_ENSURE(payload.SerializeToString(&buffer));
        return Base64Encode(buffer);
    }

    void ReturnIrrelevant() {
        Builder_.SetIrrelevant();
        BodyBuilder_.AddRenderedText("messenger_call", "irrelevant_response", NlgData_);
    }

    void ReturnDirective(TDirective directive) {
        BodyBuilder_.AddDirective(std::move(directive));
    }

    void ReturnCallerName() {
        const auto callerDeviceId = getCallerDeviceId(Request_);

        if (callerDeviceId.Defined() && IsKnownStation(*callerDeviceId)) {
            NlgData_.AddAttention("incoming_call_from_station");

        } else {
            NlgData_.Context["peer_name"] = CallState_->PeerName;
            NlgData_.Context["mics_muted"] = CallState_->MicsMuted;
        }

        BodyBuilder_.AddRenderedTextWithButtonsAndVoice("messenger_call", "who_calls_response", {}, NlgData_);
    }

    bool IsKnownStation(const TString& deviceId) {
        if (!IoTUserInfo_) {
            LOG_WARNING(Logger_) << "IoT user info not found";
            return false;
        }

        const auto& devices = IoTUserInfo_->GetIoTUserInfo().GetDevices();
        const auto it = FindIf(devices.begin(), devices.end(), [&deviceId](const auto& d) {
            return deviceId == d.GetId() || deviceId == ("device--" + d.GetId());
        });

        if (it != devices.end()) {
            const auto& platform = it->GetQuasarInfo().GetPlatform();
            return IsIn(STATIONS, platform);
        }

        return false;
    }

    void HandleIncomingNannyCall() {
        TVector<TDirective> startDirectives;
        TVector<TDirective> stopDirectives;
        ConstructStartAndStopIncomingNannyCallDirectives(startDirectives, stopDirectives);
        for (auto& directive : startDirectives) {
            BodyBuilder_.AddDirective(std::move(directive));
        }
        return ReturnAcceptCall(std::move(stopDirectives));
    }

    void ReturnAcceptCall(TVector<TDirective>&& stopDirectives = {}) {
        TDirective directive;
        directive.MutableMessengerCallDirective()->MutableAcceptCall()->SetCallGuid(CallState_->CallGuid);
        for (auto& stopDirective : stopDirectives) {
            *directive.MutableMessengerCallDirective()->MutableAcceptCall()->AddOnEnded() = std::move(stopDirective);
        }
        return ReturnDirective(std::move(directive));
    }

    bool TryConstructDrawAnimationDirective(const TStringBuf path, TDirective& directive) {
        if (!Endpoint_) {
            return false;
        }

        TAnimationCapability animationCapability;
        if (!NHollywood::ParseTypedCapability(animationCapability, *Endpoint_)) {
            return false;
        }

        const auto checkS3Url= [](const auto format) {
            return format == TAnimationCapability::S3Url;
        };
        if (AnyOf(animationCapability.GetParameters().GetSupportedFormats(), checkS3Url)) {
            directive = std::move(BuildDrawAnimationDirective(path, TAnimationCapability::TDrawAnimationDirective::SkipSpeakingAnimation));
            return true;
        }
        return false;
    }

    // It is easier to control on/off pairs when they are constructed together
    void ConstructStartAndStopIncomingNannyCallDirectives(TVector<TDirective>& startDirectives, TVector<TDirective>& stopDirectives) {
        {
            startDirectives.emplace_back();
            startDirectives.back().MutableSoundMuteDirective();
        }
        {
            stopDirectives.emplace_back();
            stopDirectives.back().MutableSoundUnmuteDirective();
        }
        
        if (!Endpoint_) {
            return;
        }

        TAnimationCapability animationCapability;
        if (NHollywood::ParseTypedCapability(animationCapability, *Endpoint_)) {
            TDirective directive;
            if (TryConstructDrawAnimationDirective(NANNY_CALL_ACCEPTED_ANIMATION_PATH, directive)) {
                startDirectives.emplace_back(std::move(directive));
            }

            for (const auto& screen : animationCapability.GetParameters().GetScreens()) {
                startDirectives.emplace_back();
                startDirectives.back().MutableDisableScreenDirective()->SetGuid(screen.GetGuid());

                stopDirectives.emplace_back();
                stopDirectives.back().MutableEnableScreenDirective()->SetGuid(screen.GetGuid());
            }
        }
    }

    void AddOutgoingNannyCallAudioFilePlayDirectives(TMessengerCallDirective::TCallToRecipient& callToRecipientDirective) {
        if (!Endpoint_) {
            return;
        }

        TAudioFilePlayerCapability audioFilePlayerCapability;
        if (!NHollywood::ParseTypedCapability(audioFilePlayerCapability, *Endpoint_)) {
            return;
        }

        for (const auto storedSound : audioFilePlayerCapability.GetParameters().GetStoredSounds()) {
            switch(storedSound) {
                case TAudioFilePlayerCapability::CallRinging: {
                    TDirective onStart;
                    onStart.MutableLocalAudioFilePlayDirective()->SetStoredSound(TAudioFilePlayerCapability::CallRinging);
                    onStart.MutableLocalAudioFilePlayDirective()->SetLooped(true);
                    BodyBuilder_.AddDirective(std::move(onStart));

                    callToRecipientDirective.AddOnAccepted()->MutableLocalAudioFileStopDirective()->SetStoredSound(TAudioFilePlayerCapability::CallRinging);
                    callToRecipientDirective.AddOnEnded()->MutableLocalAudioFileStopDirective()->SetStoredSound(TAudioFilePlayerCapability::CallRinging);
                    continue;
                }
                case TAudioFilePlayerCapability::CallEndedNannyWarning: {
                    callToRecipientDirective.AddOnEnded()->MutableLocalAudioFilePlayDirective()->SetStoredSound(TAudioFilePlayerCapability::CallEndedNannyWarning);
                    continue;
                }
                default:
                    continue;
            }
        }
    }

    void AddOutgoingNannyCallDrawAnimationDirectives(TMessengerCallDirective::TCallToRecipient& callToRecipientDirective) {
        TDirective directive;
        if (TryConstructDrawAnimationDirective(NANNY_CALL_ACCEPTED_ANIMATION_PATH, directive)) {
            *callToRecipientDirective.AddOnAccepted() = std::move(directive);
        }
    }

    const TMaybe<NAlice::TEndpoint> GetCurrentEndpoint(const TScenarioRunRequestWrapper request) {
        const NScenarios::TDataSource* dataSource = request.GetDataSource(EDataSourceType::ENVIRONMENT_STATE);
        if (!dataSource) {
            return Nothing();
        }
        const NAlice::TEnvironmentState* envState = &dataSource->GetEnvironmentState();

        if (const auto* endpoint = NHollywood::FindEndpoint(*envState, request.ClientInfo().DeviceId)) {
            return *endpoint;
        } else {
            return Nothing();
        }
    }

    void HandleOutgoingCallToOperator(const TFrame& /* frame */) {
        BodyBuilder_.CreateAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::CALL);

        BodyBuilder_.AddRenderedText("messenger_call", "call_to_operator_response", NlgData_);

        TDirective directive;
        directive.MutableMessengerCallDirective()->MutableCallToOperator();
        return ReturnDirective(std::move(directive));
    }

    void HandleOutgoingCall(const TFrame& frame) {
        BodyBuilder_.CreateAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::CALL);

        if (!Request_.Interfaces().GetOutgoingPhoneCalls() || Request_.ClientInfo().IsYaAuto()) {
            NlgData_.AddAttention("calls_not_supported_on_device");
        }

        if (!Request_.ClientInfo().IsSearchApp()) {
            NlgData_.AddAttention("messenger_calls_not_supported_on_device");
            if (Request_.Interfaces().GetSupportsShowPromo()) {
                BodyBuilder_.AddShowPromoDirective();
            }
        }

        if (OutgoingMessengerCallsEnabled_) {
            NlgData_.AddAttention("messenger_calls_enabled");
        }

        NlgData_.AddAttention("emergency_calls_enabled");

        TPtrWrapper<TSlot> slot(nullptr, "");

        if (OutgoingMessengerCallsEnabled_) {
            // Future behaviour when everything is enabled
            if (slot = frame.FindSlot("emergency")) {
                return HandleEmergencyCall(slot);

            } else if (frame.FindSlot("room") || frame.FindSlot("device") || frame.FindSlot("device_type") || frame.FindSlot("household")) {
                return HandleDeviceCall(frame);

            } else if (frame.FindSlot("other")) {
                return HandleUnknownTargetCall();

            } else {
                return HandleNoTargetCall();
            }

        } else {
            // Intermediate behaviour
            if (slot = frame.FindSlot("emergency")) {
                return HandleEmergencyCall(slot);

            } else if (frame.FindSlot("room") || frame.FindSlot("device") || frame.FindSlot("device_type") || frame.FindSlot("other")) {
                return HandleUnknownTargetCall();

            } else {
                return HandleNoTargetCall();
            }
        }

        return ReturnIrrelevant();
    }

    void HandleNoTargetCall() {
        LOG_WARNING(Logger_) << "No valid call target for MessengerCall";

        if (PhoneCall_.TryHandlePhoneCall()) {
            return;
        }

        return HandleUnknownTargetCall();
    }

    void HandleUnknownTargetCall() {
        LOG_WARNING(Logger_) << "Unknown call target for MessengerCall";

        if (PhoneCall_.TryHandlePhoneCall()) {
            return;
        }

        BodyBuilder_.AddRenderedTextWithButtonsAndVoice("messenger_call", "unknown_target_call_response", {}, NlgData_);

        if (Request_.Interfaces().GetOutgoingPhoneCalls()) {
            if (!Request_.ClientInfo().IsSmartSpeaker()) {
                AddEmergencyServiceSuggests(UserLocation_.UserCountry(), "");
            }
            AddSearchSuggest();
            return;
        }
    }

    void AddSearchSuggest() {
        const auto& query = Request_.Input().Utterance();
        BodyBuilder_.AddSearchSuggest().Title(query).Query(query);
    }

    void HandleEmergencyCall(const TPtrWrapper<TSlot>& slot) {
        const auto& emergencyService = slot->Value.AsString();

        if (!isKnownEmergencyService(emergencyService)) {
            // Should never happen
            return HandleUnknownTargetCall();

        } else {
            AddEmergencyCallIntent();
            ReturnCallToEmergencyDirective(emergencyService);
            AddSearchSuggest();
            return;
        }
    }

    void AddEmergencyCallIntent() {
        BodyBuilder_.GetAnalyticsInfoBuilder().SetIntentName(TString{EMERGENCY_CALL_INTENT});
    }

    void ReturnCallToEmergencyDirective(TStringBuf service) {
        const auto userCountry = UserLocation_.UserCountry();

        auto serviceInfo = GetEmergencyServiceInfo(userCountry, service);

        if (!Request_.ClientInfo().IsSmartSpeaker()) {
            AddEmergencyServiceSuggests(userCountry, service);
        }

        TVector<NScenarios::TLayout::TButton> buttons;

        if (IsCallPossible(serviceInfo) && !Request_.ClientInfo().IsYaAuto()) {
            serviceInfo["phone_uri"] = GeneratePhoneUri(Request_.ClientInfo(), serviceInfo["phone"], false, false);

            const auto button = CreateButton(
                "позвонить",
                "call_to_emergency",
                TString(serviceInfo["phone_uri"].GetString()),
                true
            );

            buttons.push_back(button);

            AddEmergencyCallAnalyticsInfo(serviceInfo);
        }

        NlgData_.Context["emergency_service"] = serviceInfo.ToJsonValue();
        BodyBuilder_.AddRenderedTextWithButtonsAndVoice("messenger_call", "emergency_call_response", buttons, NlgData_);
    }

    bool IsCallPossible(const NSc::TValue& recipientInfo) const {
        return recipientInfo.Has("phone") && Request_.Interfaces().GetOutgoingPhoneCalls();
    }

    void AddEmergencySuggest(const NSc::TValue& service) {
        TNlgData nlgData = NlgData_;
        nlgData.Context["emergency_service_suggest_title"] = service["title"].GetString();

        const TString caption = NlgWrapper_.RenderPhrase("messenger_call", "emergency_suggest_caption", nlgData).Text;
        const TString utterance = NlgWrapper_.RenderPhrase("messenger_call", "emergency_suggest_utterance", nlgData).Text;

        NScenarios::TDirective directive;
        auto* typeTextDirective = directive.MutableTypeTextDirective();
        typeTextDirective->SetText(utterance);
        typeTextDirective->SetName("type");

        TResponseBodyBuilder::TSuggest suggest {
            .Directives = {directive},
            .SuggestButton = caption,
        };

        BodyBuilder_.AddRenderedSuggest(std::move(suggest));
    }

    void AddEmergencyServiceSuggests(NGeobase::TId userCountry, TStringBuf serviceId) {
        const NSc::TValue& regServices = GetRegionalEmergencyServices(userCountry);

        for (const auto& name : EMERGENCY_SERVICES) {
            if (name != serviceId && regServices.Has(name)) {
                AddEmergencySuggest(regServices[name]);
            }
        }
    }

    NSc::TValue GetEmergencyServiceInfo(NGeobase::TId userCountry, TStringBuf serviceId) const {
        const NSc::TValue& regServices = GetRegionalEmergencyServices(userCountry);

        if (!regServices.Has(serviceId)) {
            serviceId = "sos";
        }

        auto result = regServices[serviceId];
        result["type"] = "emergency";
        return result;
    }

    NSc::TValue GetRegionalEmergencyServices(NGeobase::TId userCountry) const {
        const THashMap<NGeobase::TId, NSc::TValue>& phoneBook = GetEmergencyPhoneBook();
        const auto regInfo = phoneBook.find(userCountry);
        if (regInfo != phoneBook.cend()) {
            return regInfo->second;
        } else {
            return phoneBook.at(NGeobase::EARTH_REGION_ID);
        }
    }

    const THashMap<NGeobase::TId, NSc::TValue>& GetEmergencyPhoneBook() const {
        static const TPhoneBook phoneBook;
        return phoneBook.EmergencyPhoneBook;
    }

    void NoSuchDevice(const TFrame&) {
        LOG_WARNING(Logger_) << "Device call: No such device";
        AddDeviceToDeviceOpenQuasarDirective();
        BodyBuilder_.AddRenderedTextWithButtonsAndVoice("messenger_call", "call_to_device_response_open_quasar", {}, NlgData_);
    }

    void NoSuchCallableDevice(const TFrame&) {
        LOG_WARNING(Logger_) << "Device call: No such callable device";
        AddDeviceToDeviceOpenQuasarDirective();
        BodyBuilder_.AddRenderedTextWithButtonsAndVoice("messenger_call", "open_quasar_no_such_callable_device", {}, NlgData_);
    }

    bool SupportsOpenLinkOutgoingDeviceCalls() const {
        if (Request_.BaseRequestProto().GetInterfaces().GetSupportsOpenLinkOutgoingDeviceCalls()) {
            return true;
        }

        return false;
    }

    bool SupportsOutgoingDeviceCalls() const {
        if (Request_.BaseRequestProto().GetInterfaces().GetSupportsOutgoingDeviceCalls() && Request_.HasExpFlag(NExperiments::EXP_ENABLE_OUTGOING_DEVICE_TO_DEVICE_CALLS)) {
            return true;
        }

        return false;
    }

    bool CanOpenQuasarScreen() const {
        return Request_.BaseRequestProto().GetInterfaces().GetCanOpenQuasarScreen();
    }

    void HandleDeviceCall(const TFrame& frame) {
        BodyBuilder_.GetAnalyticsInfoBuilder().SetIntentName(TString{SPEAKER_CALL_INTENT});
        LOG_INFO(Logger_) << "Handling device call";

        EDeviceCallDirectiveType directiveType;
        if (SupportsOpenLinkOutgoingDeviceCalls()) {
            directiveType = EDeviceCallDirectiveType::OpenLinkDirective;
        } else if (SupportsOutgoingDeviceCalls()) {
            directiveType = EDeviceCallDirectiveType::MessengerCallDirective;
        } else {
            return HandleDeviceShortcut(frame);
        }

        if (!IoTUserInfo_) {
            LOG_WARNING(Logger_) << "Returning irrelevant: IoT user info not found";
            return NoSuchDevice(frame);
        }

        const auto devices = getTargetedDevices(frame);

        if (devices.empty()) {
            return NoSuchDevice(frame);
        }

        TVector<TDevice> callableDevices;
        CopyIf(devices.begin(), devices.end(), std::back_inserter(callableDevices), supportsCalls);

        if (callableDevices.empty()) {
            return NoSuchCallableDevice(frame);
        }

        bool nannyMode = false;
        if (frame.Name() == CALL_TO_NANNY_FRAME) {
            nannyMode = true;
        }

        return ReturnDeviceCallCard(callableDevices, directiveType, nannyMode);
    }

    void CantCallToDevice() {
        BodyBuilder_.AddRenderedTextWithButtonsAndVoice("messenger_call", "cant_call_to_device", {}, NlgData_);
    }

    static bool supportsCalls(const TDevice& device) {
        const auto& platform = device.GetQuasarInfo().GetPlatform();
        return IsIn(CALLABLE_PLATFORMS, platform);
    }

    bool isSameName(const TString& s1, const TString& s2) const {
        if (s1 == s2) {
            return true;
        }

        if (IsUtf(s1) && IsUtf(s2)) {
            return ToLowerUTF8(s1) == ToLowerUTF8(s2);
        }

        return false;
    }

    TVector<TDevice> getTargetedDevices(const TFrame& frame) const {
        const auto& devices = IoTUserInfo_->GetIoTUserInfo().GetDevices();
        const auto& rooms = IoTUserInfo_->GetIoTUserInfo().GetRooms();

        const auto& callerId = Request_.ClientInfo().DeviceId;

        TMaybe<TString> houseId;
        if (const auto slot = frame.FindSlot("household")) {
            houseId = slot->Value.AsString();
        }

        THashSet<TString> roomIds;
        if (const auto slot = frame.FindSlot("room")) {
            const TString roomId = slot->Value.AsString();

            const auto it = FindIf(rooms.begin(), rooms.end(), [roomId](const auto& r) {
                return roomId == r.GetId();
            });

            if (it != rooms.end()) {
                const TString& roomName = it->GetName();
                for (const auto& r : rooms) {
                    if (isSameName(r.GetName(), roomName)) {
                        roomIds.insert(r.GetId());
                    }
                }

            } else {
                // Should never happen
                return {};
            }
        }

        TMaybe<TString> platform;
        if (const auto slot = frame.FindSlot("device_type")) {
            platform = slot->Value.AsString();
        }

        TMaybe<TString> name;
        if (const auto slot = frame.FindSlot("device")) {
            const TString deviceId = slot->Value.AsString();

            const auto it = FindIf(devices.begin(), devices.end(), [deviceId](const auto& d) {
                return deviceId == d.GetId() || deviceId == ("device--" + d.GetId());
            });

            if (it != devices.end()) {
                name = it->GetName();
            } else {
                // Should never happen
                return {};
            }
        }
        TVector<TDevice> targetedDevices;
        LOG_INFO(Logger_) << "houseId = " << houseId << "roomIds.size() = " << roomIds.size() << ", " << "name = " << name << ", platform = " << platform;
        CopyIf(devices.begin(), devices.end(), std::back_inserter(targetedDevices), [this, &houseId, &roomIds, &name, &platform, &callerId](const auto& d) {
            if (houseId.Defined() && houseId != d.GetHouseholdId()) {
                return false;
            }

            if (!roomIds.empty() && !roomIds.contains(d.GetRoomId())) {
                return false;
            }

            if (name.Defined() && !isSameName(*name, d.GetName())) {
                return false;
            }

            if (platform.Defined() && *platform != "any" && d.GetQuasarInfo().GetPlatform() != *platform) {
                return false;
            }

            if (d.GetQuasarInfo().GetDeviceId() == callerId) {
                return false;
            }

            return true;
        });
        LOG_INFO(Logger_) << "Matched devices count: " << targetedDevices.size();
        return targetedDevices;
    }

    NScenarios::TLayout::TButton CreateButton(const TString& title, const TString& actionId, NScenarios::TDirective&& directive, bool addToDirectives = false) {
        NScenarios::TFrameAction action;
        *action.MutableDirectives()->AddList() = directive;
        auto& nluInstance = *action.MutableNluHint()->AddInstances();
        nluInstance.SetPhrase(title);
        nluInstance.SetLanguage(Request_.Proto().GetBaseRequest().GetUserLanguage());

        BodyBuilder_.AddAction(actionId, std::move(action));

        NScenarios::TLayout::TButton button;
        button.SetTitle(title);
        button.SetActionId(actionId);

        if (addToDirectives) {
            BodyBuilder_.AddDirective(std::move(directive));
        }

        return button;
    }

    NScenarios::TLayout::TButton CreateButton(const TString& title, const TString& actionId, const TString& uri, bool addToDirectives = false) {
        NScenarios::TDirective directive;
        directive.MutableOpenUriDirective()->SetUri(uri);
        return CreateButton(title, actionId, std::move(directive), addToDirectives);
    }

    void AddDeviceToDeviceSingleDirective(const TDevice& device, const EDeviceCallDirectiveType& directiveType, const bool nannyMode) {
        const auto deviceId = device.GetQuasarInfo().GetDeviceId();
        auto directive = GetDeviceCallDirective(deviceId, GetFullDeviceName(device), directiveType, nannyMode);
        BodyBuilder_.AddDirective(std::move(directive));
    }

    void AddDeviceToDeviceOpenQuasarDirective() {
        if (CanOpenQuasarScreen()) {
            NScenarios::TDirective directive;
            directive.MutableOpenUriDirective()->SetUri(TString{DEVICE_SHORTCUT_URL});
            BodyBuilder_.AddDirective(std::move(directive));
        } else {
            TPushDirectiveBuilder {
                DEVICE_CALL_PUSH_TITLE,
                DEVICE_CALL_PUSH_TEXT,
                DEVICE_CALL_PUSH_URL,
                DEVICE_CALL_PUSH_TAG
            }
                .SetThrottlePolicy(DEVICE_CALL_PUSH_POLICY)
                .BuildTo(BodyBuilder_);

            NlgData_.AddAttention("push_open_quasar_screen");
        }
    }

    NScenarios::TDirective GetDeviceCallDirective(const TString& deviceId,
                                                  const TString& deviceName,
                                                  const EDeviceCallDirectiveType& directiveType,
                                                  const bool nannyMode = false)
    {
        NScenarios::TDirective directive;
        switch (directiveType) {
            case EDeviceCallDirectiveType::OpenLinkDirective:
                directive.MutableOpenUriDirective()->SetUri(GetDeviceCallUrl(deviceId, deviceName));
                return directive;
            case EDeviceCallDirectiveType::MessengerCallDirective:
                auto* callToRecipient = directive.MutableMessengerCallDirective()->MutableCallToRecipient();
                callToRecipient->MutableRecipient()->SetDeviceId(deviceId);
                if (nannyMode) {
                    AddOutgoingNannyCallAudioFilePlayDirectives(*callToRecipient);
                    AddOutgoingNannyCallDrawAnimationDirectives(*callToRecipient);
                    callToRecipient->SetPayload(SerializeToCallPayload(nannyMode));
                }
                return directive;
        }
    }

    void AddEmergencyCallAnalyticsInfo(const NSc::TValue& serviceInfo) {
        BodyBuilder_.GetAnalyticsInfoBuilder().AddAction(
            "call.emergency_call",
            "call to emergency service",
            TStringBuilder{} << "Осуществляется звонок выбранному абоненту: \"" << serviceInfo.Get("title").GetString() << "\""
        );
    }

    void AddDeviceCallAnalyticsInfo(const TVector<TDevice>& devices) {
        auto& builder = BodyBuilder_.GetAnalyticsInfoBuilder();

        if (devices.size() == 1) {
            builder.AddAction(
                "call.device_call",
                "call to device",
                "Совершается звонок в колонку"
            );
        } else {
            builder.AddAction(
                "call.show_callable_devices_table",
                "show callable devices table",
                "Появляется таблица с выбором устройства для звонка"
            );
        }

        for (const auto& device : devices) {
            builder.AddObject(
                "callee_device",
                "call target device",
                TStringBuilder{} << GetFullDeviceName(device) << " (" << device.GetQuasarInfo().GetDeviceId() << ")"
            );
        }
    }

    void ReturnDeviceCallCard(const TVector<TDevice>& devices, const EDeviceCallDirectiveType& directiveType, const bool nannyMode) {
        TVector<NScenarios::TLayout::TButton> buttons;

        if (devices.size() == 1) {
            LOG_INFO(Logger_) << "Device call: executing single call";
            const auto& device = devices.front();
            const auto deviceName = GetFullDeviceName(device);

            AddDeviceToDeviceSingleDirective(device, directiveType, nannyMode);

            NlgData_.Context["target_device"] = deviceName;
            BodyBuilder_.AddRenderedTextWithButtonsAndVoice("messenger_call", "call_to_device_response_single", buttons, NlgData_);
            AddDeviceCallAnalyticsInfo(devices);
            return;
        }

        LOG_INFO(Logger_) << "Device call: opening quasar screen";
        AddDeviceToDeviceOpenQuasarDirective();

        BodyBuilder_.AddRenderedTextWithButtonsAndVoice("messenger_call", "call_to_device_response_open_quasar", {}, NlgData_);
        AddDeviceCallAnalyticsInfo(devices);
    }

    TString GetDeviceCallUrl(const TStringBuf deviceId, const TStringBuf title) const {
        TString titleQuoted(title);
        Quote(titleQuoted, "");

        TStringBuilder url;
        url << "messenger://call/create/private?device_id=" << deviceId << "&title=" << titleQuoted;
        return url;
    }

    TString GetDevicesScreenUrl() const {
        return "ya-search-app-open://?uri=yellowskin%3A%2F%2F%3Furl%3Dhttps%253A%252F%252Fyandex.ru%252Fquasar%252Fiot";
    }

    THashMap<TString, TString> GetSmartHomeRooms() const {
        THashMap<TString, TString> rooms;

        if (IoTUserInfo_) {
            for (const auto& r : IoTUserInfo_->GetIoTUserInfo().GetRooms()) {
                rooms[r.GetId()] = r.GetName();
            }
        }

        return rooms;
    }

    TString GetFullDeviceName(const TDevice& device) const {
        TStringBuilder name;

        name << device.GetName();

        if (!device.GetRoomId().empty()) {
            const auto it = Rooms_.find(device.GetRoomId());

            if (it != Rooms_.end()) {
                name << " - " << it->second;
            }
        }

        return name;
    }

private:
    TRTLogger& Logger_;
    const TScenarioRunRequest RequestProto_;
    const TScenarioRunRequestWrapper Request_;
    TNlgWrapper NlgWrapper_;
    TRunResponseBuilder Builder_;
    TResponseBodyBuilder& BodyBuilder_;
    const TUserLocation UserLocation_;
    TNlgData NlgData_;
    const TMaybe<TCallState> CallState_;
    const NScenarios::TDataSource* IoTUserInfo_;
    const THashMap<TString, TString> Rooms_;
    const NGeobase::TLookup& Geobase_;
    const TCallbackDirective* Callback_;
    const bool OutgoingMessengerCallsEnabled_;
    const bool OutgoingOperatorCallsEnabled_;
    const bool DeviceCallShortcutEnabled_;
    TState State_;
    TPhoneCall PhoneCall_;
    const TMaybe<NAlice::TEndpoint> Endpoint_;
};

TMessengerCall::TPhoneBook::TPhoneBook() {
    TString content;
    if (!NResource::FindExact(PHONEBOOK_FILENAME, &content)) {
        ythrow yexception() << "Unable to load built-in resource " << PHONEBOOK_FILENAME;
    }

    TStringBuilder validationErrorMessage;
    validationErrorMessage << "Incorrect data in " << PHONEBOOK_FILENAME << ": ";
    NSc::TValue json = NSc::TValue::FromJson(content);
    const NSc::TDict& dict = json.GetDict();
    for (const auto& record : dict) {
        NGeobase::TId country = FromString<NGeobase::TId>(record.first.data(), record.first.length(), NGeobase::UNKNOWN_REGION);
        if (!NAlice::IsValidId(country)) {
            ythrow yexception() << validationErrorMessage
                                << record.first << " is not valid country geo id";
        }
        // every country record should contain SOS_SERVICE_ID phone
        if (!record.second.Has(SOS_SERVICE_ID)) {
            ythrow yexception() << validationErrorMessage
                                << "<" << SOS_SERVICE_ID << "> phone is not defined for the country " << country;
        }
        EmergencyPhoneBook[country] = record.second;
    }
}

} // namespace

void TMessengerCallRunHandle::Do(TScenarioHandleContext& ctx) const {
    ctx.ServiceCtx.AddProtobufItem(*TMessengerCall{ctx}.MakeResponse(), RESPONSE_ITEM);
}

REGISTER_SCENARIO("messenger_call", AddHandle<TMessengerCallRunHandle>().SetNlgRegistration(
                                        NAlice::NHollywood::NLibrary::NScenarios::NMessengerCall::NNlg::RegisterAll));

} // namespace NAlice::NHollywood

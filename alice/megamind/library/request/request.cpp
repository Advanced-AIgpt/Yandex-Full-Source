#include "request.h"

#include "builder.h"

#include <alice/megamind/protos/common/device_state.pb.h>
#include <alice/megamind/protos/common/location.pb.h>
#include <alice/megamind/protos/guest/enrollment_headers.pb.h>
#include <alice/megamind/protos/quasar/auxiliary_config.pb.h>

#include <alice/megamind/library/biometry/biometry.h>
#include <alice/megamind/library/common/defs.h>
#include <alice/megamind/library/experiments/flags.h>
#include <alice/megamind/library/models/directives/callback_directive_model.h>
#include <alice/megamind/library/scenarios/defs/names.h>
#include <alice/megamind/library/util/request.h>
#include <alice/megamind/library/util/ttl.h>

#include <alice/library/client/client_features.h>
#include <alice/library/client/client_info.h>
#include <alice/library/experiments/experiments.h>
#include <alice/library/experiments/flags.h>
#include <alice/library/json/json.h>
#include <alice/library/music/defs.h>
#include <alice/library/proto/protobuf.h>

namespace NAlice {

namespace {

EScenarioNameSource ConvertToVinsSource(const EScenarioNameSource source) {
    switch (source) {
        case EScenarioNameSource::VinsDialogId:
            [[fallthrough]];
        case EScenarioNameSource::DialogId:
            return EScenarioNameSource::VinsDialogId;
        case EScenarioNameSource::VinsServerAction:
            [[fallthrough]];
        case EScenarioNameSource::ServerAction:
            return EScenarioNameSource::VinsServerAction;
    }
}

TString ExtractStringValue(const google::protobuf::Struct& data, const TString& jsonKey) {
    const auto& fields = data.fields();
    if (!fields.count(jsonKey)) {
        return "";
    }
    return fields.at(jsonKey).string_value();
}

TMaybe<TRequest::TLocation> TryParseLaasLocation(const TMaybe<google::protobuf::Struct>& laasRegion) {
    // See: https://a.yandex-team.ru/arc/trunk/arcadia/alice/bass/forms/context/context.cpp?rev=6445189#L55
    if (!laasRegion.Defined()) {
        return Nothing();
    }
    const auto& rawLaasRegion = laasRegion->fields();
    for (const auto& field : {"latitude", "longitude", "location_accuracy", "location_unixtime"}) {
        if (!rawLaasRegion.count(field)) {
            return Nothing();
        }
    }
    const auto laasTimestamp = rawLaasRegion.at("location_unixtime").number_value();
    const auto currentTime = TInstant::Now().Seconds();
    double recency = 0;
    if (currentTime > laasTimestamp) {
        recency = TDuration::Seconds(currentTime - laasTimestamp).MilliSeconds();
    }
    return TRequest::TLocation{
        rawLaasRegion.at("latitude").number_value(),
        rawLaasRegion.at("longitude").number_value(),
        rawLaasRegion.at("location_accuracy").number_value(),
        recency,
    };
}

TMaybe<TRequest::TLocation> ParseLocationFromSkr(const TSpeechKitRequest& speechKitRequest) {
    const auto& request = speechKitRequest->GetRequest();
    return NMegamind::ParseLocation(request.HasLocation()
                                        ? request.GetLocation()
                                        : TMaybe<NAlice::TLocation>{},
                                    request.HasLaasRegion()
                                        ? request.GetLaasRegion()
                                        : TMaybe<google::protobuf::Struct>{});
}

EContentRestrictionLevel ParseContentRestrictionLevel(const TSpeechKitRequestProto_TRequest& request,
                                                      bool isClassifiedAsChildRequest = false) {
    const auto& bassOptions = request.GetAdditionalOptions().GetBassOptions();

    const auto bassFiltrationLevel =
        bassOptions.HasFiltrationLevel() ? TMaybe<ui32>(bassOptions.GetFiltrationLevel()) : Nothing();

    const auto& deviceConfig = request.GetDeviceState().GetDeviceConfig();
    const auto& contentSettings = deviceConfig.GetContentSettings();

    if (isClassifiedAsChildRequest) {
        if (deviceConfig.HasChildContentSettings()) {
            const auto& childContentSettings = deviceConfig.GetChildContentSettings();
            const auto restrictionLevel = CalculateContentRestrictionLevel(childContentSettings, bassFiltrationLevel);
            return GetContentRestrictionLevel(restrictionLevel);
        }
        return EContentRestrictionLevel::Children;
    }

    const auto& restrictionLevel = CalculateContentRestrictionLevel(contentSettings, bassFiltrationLevel);
    return GetContentRestrictionLevel(restrictionLevel);
}

NScenarios::TUserPreferences::EFiltrationMode GetFiltrationMode(const EContentRestrictionLevel level) {
    switch (level) {
        case EContentRestrictionLevel::Without:
            return NScenarios::TUserPreferences_EFiltrationMode_NoFilter;
        case EContentRestrictionLevel::Medium:
            return NScenarios::TUserPreferences_EFiltrationMode_Moderate;
        case EContentRestrictionLevel::Children:
            return NScenarios::TUserPreferences_EFiltrationMode_FamilySearch;
        case EContentRestrictionLevel::Safe:
            return NScenarios::TUserPreferences_EFiltrationMode_Safe;
    }
}

bool IsMusicServerAction(const TEvent& event) {
    if (event.GetName() != "bass_action") {
        return false;
    }

    const auto& fields = event.GetPayload().fields();
    if (const auto nameIter = fields.find("name"); nameIter != fields.end()) {
        const auto& name = nameIter->second;
        return name.string_value() == NMusic::MUSIC_PLAY_OBJECT;
    }

    return false;
}

// TODO (petrk) Get rid of this hack when reminders scenario moved from bass to hw.
// MEGAMIND-2892
bool IsRemindersServerAction(const TEvent& event) {
    if (!event.GetName().StartsWith("reminders_")) {
        return false;
    }
    const auto& fields = event.GetPayload().fields();
    const auto it = fields.find(NMegamind::SCENARIO_NAME_JSON_KEY);
    return it == fields.cend() || it->second.string_value() == MM_PROTO_VINS_SCENARIO;
}

bool IsTodayWeatherServerAction(const TEvent& event) {
    if (event.GetName() != "update_form") {
        return false;
    }

    const auto& fields = event.GetPayload().fields();
    if (const auto formUpdateIter = fields.find("form_update"); formUpdateIter != fields.end()) {
        const auto& formUpdateFields = formUpdateIter->second.struct_value().fields();

        const auto pushIdIter = formUpdateFields.find("push_id");
        const auto nameIter = formUpdateFields.find("name");

        if (pushIdIter == formUpdateFields.end() || nameIter == formUpdateFields.end()) {
            return false;
        }

        const auto& pushId = pushIdIter->second;
        if (pushId.string_value() != "weather_today") {
            return false;
        }

        const auto& name = nameIter->second;
        if (name.string_value() != "personal_assistant.scenarios.get_weather") {
            return false;
        }

        return true;
    }

    return false;
}

} // namespace

const TMaybe<TString>& TRequest::GetDialogId() const {
    return DialogId;
}

const TMaybe<TString>& TRequest::GetScenarioDialogId() const {
    return ScenarioDialogId;
}

const TMaybe<TRequest::TScenarioInfo>& TRequest::GetScenario() const {
    return Scenario;
}

NMegamind::TRequestData TRequest::ToProto() const {
    NMegamind::TRequestData requestData;
    requestData.MutableEvent()->CopyFrom(Event->SpeechKitEvent());

    if (DialogId.Defined()) {
        requestData.SetDialogId(*DialogId);
    }
    if (ScenarioDialogId.Defined()) {
        requestData.SetScenarioDialogId(*ScenarioDialogId);
    }

    if (Scenario.Defined()) {
        auto& scenario = *requestData.MutableScenario();
        scenario.SetName(Scenario->GetName());
        switch (Scenario->GetSource()) {
            case EScenarioNameSource::DialogId:
                scenario.SetSource(NMegamind::TRequestData_TScenarioInfo_EScenarioNameSource_DialogId);
                break;
            case EScenarioNameSource::ServerAction:
                scenario.SetSource(NMegamind::TRequestData_TScenarioInfo_EScenarioNameSource_ServerAction);
                break;
            case EScenarioNameSource::VinsDialogId:
                scenario.SetSource(NMegamind::TRequestData_TScenarioInfo_EScenarioNameSource_VinsDialogId);
                break;
            case EScenarioNameSource::VinsServerAction:
                scenario.SetSource(NMegamind::TRequestData_TScenarioInfo_EScenarioNameSource_VinsServerAction);
                break;
        }
    }

    if (Location.Defined()) {
        auto& loc = *requestData.MutableLocation();
        loc.SetLatitude(Location->GetLatitude());
        loc.SetLongitude(Location->GetLongitude());
        loc.SetAccuracy(Location->GetAccuracy());
        loc.SetRecency(Location->GetRecency());
        loc.SetSpeed(Location->GetSpeed());
    }

    auto& userLoc = *requestData.MutableUserLocation();
    userLoc.SetUserRegion(UserLocation.UserRegion());
    userLoc.SetUserCountry(UserLocation.UserCountry());
    userLoc.SetUserTld(UserLocation.UserTld());
    userLoc.SetUserTimeZone(UserLocation.UserTimeZone());

    switch (ContentRestrictionLevel) {
        case EContentRestrictionLevel::Medium:
            requestData.SetContentRestrictionLevel(NMegamind::TRequestData_EContentRestrictionLevel_Medium);
            break;
        case EContentRestrictionLevel::Children:
            requestData.SetContentRestrictionLevel(NMegamind::TRequestData_EContentRestrictionLevel_Children);
            break;
        case EContentRestrictionLevel::Without:
            requestData.SetContentRestrictionLevel(NMegamind::TRequestData_EContentRestrictionLevel_Without);
            break;
        case EContentRestrictionLevel::Safe:
            requestData.SetContentRestrictionLevel(NMegamind::TRequestData_EContentRestrictionLevel_Safe);
            break;
    }

    for (const auto& frame : SemanticFrames) {
        requestData.AddSemanticFrames()->CopyFrom(frame);
    }

    if (!RecognizedActionEffectFrames.empty()) {
        requestData.MutableRecognizedActionEffectFrames()->CopyFrom({RecognizedActionEffectFrames.begin(), RecognizedActionEffectFrames.end()});
    }

    requestData.MutableStackEngineCore()->CopyFrom(StackEngineCore);
    requestData.SetServerTimeMs(ServerTimeMs);
    requestData.MutableInterfaces()->CopyFrom(Interfaces);
    requestData.MutableOptions()->CopyFrom(Options);
    requestData.MutableUserPreferences()->CopyFrom(UserPreferences);
    requestData.MutableUserClassification()->CopyFrom(UserClassification);

    if (const auto& forcedShouldListen = Parameters.GetForcedShouldListen(); forcedShouldListen.Defined()) {
        requestData.MutableParameters()->SetForcedShouldListen(*forcedShouldListen);
    }
    if (const auto& directiveChannel = Parameters.GetChannel(); directiveChannel.Defined()) {
        requestData.MutableParameters()->SetChannel(*directiveChannel);
    }
    if (const auto& forcedEmotion = Parameters.GetForcedEmotion(); forcedEmotion.Defined()) {
        requestData.MutableParameters()->SetForcedEmotion(*forcedEmotion);
    }

    requestData.SetRequestSource(RequestSource);

    if (IotUserInfo.Defined()) {
        requestData.MutableIoTUserInfo()->CopyFrom(*IotUserInfo);
    }

    if (ContactsList.Defined()) {
        requestData.MutableContactsList()->CopyFrom(*ContactsList);
    }

    if (Origin.Defined()) {
        requestData.MutableOrigin()->CopyFrom(*Origin);
    }

    if (WhisperInfo.Defined()) {
        *requestData.MutableWhisperInfo() = WhisperInfo->ToProto();
    }

    if (CallbackOwnerScenario.Defined()) {
        *requestData.MutableCallbackOwnerScenario() = *CallbackOwnerScenario;
    }

    requestData.SetIsWarmUp(IsWarmUp_);

    for (const auto& frame : AllParsedSemanticFrames) {
        requestData.AddAllParsedSemanticFrames()->CopyFrom(frame);
    }
    if (GuestData.Defined()) {
        *requestData.MutableGuestData() = *GuestData;
    }

    if (GuestOptions.Defined()) {
        *requestData.MutableGuestOptions() = *GuestOptions;
    }

    requestData.SetDisableVoiceSession(DisableVoiceSession);
    requestData.SetDisableShouldListen(DisableShouldListen);

    return requestData;
}

// TRequest::TScenarioInfo -----------------------------------------------------
TRequest::TScenarioInfo::TScenarioInfo(const TString& name, EScenarioNameSource source) {
    if (name.empty()) {
        Name = TString{MM_VINS_SCENARIO};
        Source = ConvertToVinsSource(source);
    } else {
        Name = name;
        Source = source;
    }
}

const TString& TRequest::TScenarioInfo::GetName() const {
    return Name;
}

const EScenarioNameSource& TRequest::TScenarioInfo::GetSource() const {
    return Source;
}

// TRequest::TWhisperInfo--------------------------------------------------------
TRequest::TWhisperInfo::TWhisperInfo(bool isVoiceInput, ui64 lastWhisperTimeMs, ui64 serverTimeMs, ui64 whisperTtlMs,
                                     bool isAsrWhisper, const TMaybe<TTtsWhisperConfig>& whisperConfig)
    : IsVoiceInput_{isVoiceInput}
    , LastWhisperTimeMs_{lastWhisperTimeMs}
    , ServerTimeMs_{serverTimeMs ? serverTimeMs : TInstant::Now().MilliSeconds()}
    , WhisperTtlMs_{whisperTtlMs}
    , IsAsrWhisper_{isAsrWhisper}
    , WhisperConfig_{whisperConfig} {
}

ui64 TRequest::TWhisperInfo::GetUpdatedLastWhisperTimeMs() const {
    if (WhisperConfig_.Defined() && !WhisperConfig_->GetEnabled()) {
        return 0;
    }
    if (IsVoiceInput_) {
        return IsAsrWhisper_ ? ServerTimeMs_ : 0;
    }
    return LastWhisperTimeMs_;
}

bool TRequest::TWhisperInfo::IsPreviousRequestWhisper() const {
    return WhisperTtlMs_ != 0 && !NMegamind::IsTimeoutExceeded(LastWhisperTimeMs_, WhisperTtlMs_, ServerTimeMs_);
}

bool TRequest::TWhisperInfo::IsWhisper() const {
    if (WhisperConfig_.Defined() && !WhisperConfig_->GetEnabled()) {
        return false;
    }
    if (IsAsrWhisper_) {
        return true;
    }
    return !IsVoiceInput_ && IsPreviousRequestWhisper();
}

bool TRequest::TWhisperInfo::IsAsrWhisper() const {
    return IsAsrWhisper_;
}

NMegamind::TRequestData_TWhisperInfo TRequest::TWhisperInfo::ToProto() const {
    NMegamind::TRequestData_TWhisperInfo proto;
    proto.SetIsVoiceInput(IsVoiceInput_);
    proto.SetLastWhisperTimeMs(LastWhisperTimeMs_);
    proto.SetWhisperTtlMs(WhisperTtlMs_);
    proto.SetIsAsrWhisper(IsAsrWhisper_);
    if (WhisperConfig_.Defined()) {
        *proto.MutableWhisperConfig() = *WhisperConfig_;
    }
    return proto;
}

namespace NMegamind {

namespace {

struct TScenarioInfoWithTabs final {
    TMaybe<TString> DialogId;
    TMaybe<TString> ScenarioDialogId;

    TMaybe<TRequest::TScenarioInfo> Scenario;
};

TRequest::TWhisperInfo ParseWhisperInfo(const TSpeechKitRequest& rawRequest,
                                        const std::unique_ptr<const IEvent>& event, ui64 lastWhisperTimeMs,
                                        ui64 whisperTtlMs, const TMaybe<TRequest::TTtsWhisperConfig>& whisperConfig,
                                        TRTLogger& logger) {
    auto whisperInfo = TRequest::TWhisperInfo{event->IsVoiceInput(),
                                              lastWhisperTimeMs,
                                              rawRequest->GetRequest().GetAdditionalOptions().GetServerTimeMs(),
                                              whisperTtlMs,
                                              event->IsAsrWhisper(),
                                              whisperConfig};
    LOG_INFO(logger) << "IsWhisper:" << whisperInfo.IsWhisper()
                     << ", WhisperInfoProto: " << whisperInfo.ToProto().ShortUtf8DebugString();
    return whisperInfo;
}

TScenarioInfoWithTabs ParseScenarioInfo(const TSpeechKitRequest& speechKitRequest,
                                        const std::unique_ptr<const IEvent>& requestEvent) {
    TScenarioInfoWithTabs scenarioInfo{};
    if (const auto& dialogId = speechKitRequest->GetHeader().GetDialogId()) {
        scenarioInfo.DialogId = dialogId;
        auto split = NMegamind::SplitDialogId(dialogId);
        if (split.ScenarioName.empty()) {
            scenarioInfo.Scenario = {/* name= */ TString{MM_DIALOGOVO_SCENARIO},
                                     /* source= */ EScenarioNameSource::VinsDialogId};
        } else {
            scenarioInfo.Scenario = {/* name= */ split.ScenarioName, /* source= */ EScenarioNameSource::DialogId};
        }
        scenarioInfo.ScenarioDialogId = split.ScenarioDialogId;
    }

    if (const auto& event = requestEvent->SpeechKitEvent(); requestEvent->AsServerActionEvent()) {
        const auto scenarioNameKey = TString{NMegamind::SCENARIO_NAME_JSON_KEY};
        // For some reason sk clients create "new_dialog_session" on device's side when user opens new tab
        // Fortunately, they provide "DialogId" information during such requests
        //  that's why we don't rewrite Scenario if we observe "new_dialog_session" created on client side
        if (event.GetName() == "new_dialog_session" && !event.GetPayload().fields().count(scenarioNameKey)) {
            if (scenarioInfo.Scenario.Empty()) {
                scenarioInfo.Scenario = {/* name= */ TString{MM_DIALOGOVO_SCENARIO},
                                         /* source= */ EScenarioNameSource::VinsServerAction};
            }
        } else if (IsTodayWeatherServerAction(event)) {
            // Old Yandex.Phone (currently out of development) has a legacy hardcoded weather callback:
            // https://a.yandex-team.ru/arc/trunk/arcadia/mobile/launcher/android/launcher-app/launcher/src/main/java/com/android/launcher3/contextcards/WeatherCardController.java?rev=r7772064#L48-62
            // We can't change the firmware, so we need to re-route this callback
            scenarioInfo.Scenario = {/* name= */ TString{HOLLYWOOD_WEATHER_SCENARIO},
                                     /* source= */ EScenarioNameSource::ServerAction};
        } else if (IsMusicServerAction(event) &&
                   speechKitRequest.HasExpFlag(NExperiments::EXP_HOLLYWOOD_MUSIC_SERVER_ACTION)) {
            scenarioInfo.Scenario = {/* name= */ TString{HOLLYWOOD_MUSIC_SCENARIO},
                                     /* source= */ EScenarioNameSource::ServerAction};
        } else if (IsRemindersServerAction(event)) {
            scenarioInfo.Scenario = {/* name= */ TString{HOLLYWOOD_REMINDERS_SCENARIO},
                                     /* source= */ EScenarioNameSource::ServerAction};
        } else {
            scenarioInfo.Scenario = {/* name= */ ExtractStringValue(event.GetPayload(), scenarioNameKey),
                                     /* source= */ EScenarioNameSource::ServerAction};
        }
    }

    if (scenarioInfo.Scenario && scenarioInfo.Scenario->GetName() == MM_VINS_SCENARIO &&
        NMegamind::IsProtoVinsEnabled(speechKitRequest)) {
        scenarioInfo.Scenario = {/* name= */ TString{MM_PROTO_VINS_SCENARIO},
                                 /* source= */ scenarioInfo.Scenario->GetSource()};
    }

    return scenarioInfo;
}

TRequest::TInterfaces ParseInterfacesFromSkr(const TSpeechKitRequest& speechKitRequest) {
    return ParseInterfaces(speechKitRequest->GetRequest().GetExperiments(),
                           speechKitRequest->GetRequest().GetAdditionalOptions().GetSupportedFeatures(),
                           speechKitRequest->GetRequest().GetAdditionalOptions().GetUnsupportedFeatures(),
                           speechKitRequest->GetApplication(),
                           speechKitRequest->GetRequest().GetDeviceState().GetIsTvPluggedIn(),
                           speechKitRequest.GetVoiceSession());
}

TRequest::TOptions ParseOptions(const TSpeechKitRequest& speechKitRequest) {
    const auto& additionalOptions = speechKitRequest->GetRequest().GetAdditionalOptions();
    const auto& bassOptions = additionalOptions.GetBassOptions();

    TRequest::TOptions options{};

    options.SetUserAgent(bassOptions.GetUserAgent());
    // filtration_level == 1 is the same as no filtration level, according to CalculateContentRestrictionLevel
    options.SetFiltrationLevel(bassOptions.HasFiltrationLevel() ? bassOptions.GetFiltrationLevel() : 1);
    options.SetClientIP(bassOptions.GetClientIP());
    // 0 is an acceptable default, currently BASS is matching factors against a white list with lower_bound
    options.SetScreenScaleFactor(bassOptions.GetScreenScaleFactor());
    options.SetVideoGalleryLimit(std::clamp(bassOptions.GetVideoGalleryLimit(), 0, UINT8_MAX));
    options.SetRawPersonalData(speechKitRequest->GetRequest().GetRawPersonalData());
    options.MutableRadioStations()->MergeFrom(additionalOptions.GetRadioStations());
    if (bassOptions.HasRegionId()) {
        options.SetUserDefinedRegionId(bassOptions.GetRegionId());
    }
    if (additionalOptions.HasQuasarAuxiliaryConfig()) {
        options.MutableQuasarAuxiliaryConfig()->CopyFrom(additionalOptions.GetQuasarAuxiliaryConfig());
    }
    options.MutableFavouriteLocations()->CopyFrom(additionalOptions.GetFavouriteLocations());

    auto& permissions = *options.MutablePermissions();
    for (const auto& srcPermission : additionalOptions.GetPermissions()) {
        auto& permission = *permissions.Add();
        permission.SetName(srcPermission.GetName());
        permission.SetGranted(srcPermission.GetGranted() || srcPermission.GetStatus());
    }
    if (const auto& megamindCookies = speechKitRequest->GetRequest().GetMegamindCookies(); !megamindCookies.empty()) {
        google::protobuf::util::JsonStringToMessage(megamindCookies, options.MutableMegamindCookies());
    }
    options.SetCanUseUserLogs(!additionalOptions.GetDoNotUseUserLogs());
    options.SetPromoType(TClientInfo(speechKitRequest.ClientInfoProto()).PromoType);

    return options;
}

TRequest::TUserPreferences ParseUserPreferences(EContentRestrictionLevel contentRestrictionLevel) {
    TRequest::TUserPreferences preferences{};
    preferences.SetFiltrationMode(GetFiltrationMode(contentRestrictionLevel));
    return preferences;
}

TMaybe<TGuestOptions> ParseGuestOptions(const TSpeechKitRequest& speechKitRequest) {
    auto hasGuestUserOptions = speechKitRequest->GetRequest().GetAdditionalOptions().HasGuestUserOptions();
    auto hasEnrollmentHeaders = speechKitRequest->HasEnrollmentHeaders();
    if (!hasGuestUserOptions && !hasEnrollmentHeaders) {
        return Nothing();
    }

    TGuestOptions guestOptions;
    if (hasGuestUserOptions) {
        guestOptions = speechKitRequest->GetRequest().GetAdditionalOptions().GetGuestUserOptions();
    } else {
        guestOptions.SetStatus(TGuestOptions::NoMatch);
    }

    auto isOwnerEnrolled = false;
    if (hasEnrollmentHeaders) {
        for (const auto& header : speechKitRequest->GetEnrollmentHeaders().GetHeaders()) {
            if (header.GetUserType() == OWNER && !header.GetPersonId().Empty()) {
                isOwnerEnrolled = true;
                break;
            }
        }
    }
    guestOptions.SetIsOwnerEnrolled(isOwnerEnrolled);

    return std::move(guestOptions);
}

TRequestBuilder CreateRequestBuilder(std::unique_ptr<const IEvent> event, const TSpeechKitRequest& rawRequest,
                                     const TVector<TSemanticFrame>& semanticFrames,
                                     const TVector<TSemanticFrame>& recognizedActionEffectFrames,
                                     const TStackEngineCore& stackEngineCore,
                                     const TRequest::TParameters& parameters,
                                     NScenarios::TScenarioBaseRequest::ERequestSourceType requestSource,
                                     const TMaybe<TIoTUserInfo>& iotUserInfo,
                                     const TMaybe<NAlice::NData::TContactsList>& contactsList,
                                     const TMaybe<TOrigin>& origin, ui64 lastWhisperTimeMs, ui64 whisperTtlMs,
                                     const TMaybe<TString>& callbackOwnerScenario,
                                     const TMaybe<TRequest::TTtsWhisperConfig>& whisperConfig,
                                     TRTLogger& logger,
                                     bool isWarmUp,
                                     const TVector<TSemanticFrame>& allParsedSemanticFrames,
                                     bool disableVoiceSession, bool disableShouldListen) {
    const auto scenarioInfoView = ParseScenarioInfo(rawRequest, event);
    const auto whisperInfo =
        ParseWhisperInfo(rawRequest, event, lastWhisperTimeMs, whisperTtlMs, whisperConfig, logger);
    const auto userClassification = ParseUserClassification(rawRequest.Event().GetBiometryClassification());
    const auto contentRestrictionLevel = ParseContentRestrictionLevel(
        rawRequest->GetRequest(), userClassification.GetAge() == TRequest::TUserClassification::Child);
    const auto guestOptions = ParseGuestOptions(rawRequest);

    TRequestBuilder builder{std::move(event)};
    builder.SetLocation(ParseLocationFromSkr(rawRequest))
        .SetScenario(scenarioInfoView.Scenario)
        .SetDialogId(scenarioInfoView.DialogId)
        .SetScenarioDialogId(scenarioInfoView.ScenarioDialogId)
        .SetSemanticFrames(semanticFrames)
        .SetRecognizedActionEffectFrames(recognizedActionEffectFrames)
        .SetContentRestrictionLevel(contentRestrictionLevel)
        .SetStackEngineCore(stackEngineCore)
        .SetServerTimeMs(rawRequest->GetRequest().GetAdditionalOptions().GetServerTimeMs())
        .SetInterfaces(ParseInterfacesFromSkr(rawRequest))
        .SetOptions(ParseOptions(rawRequest))
        .SetUserPreferences(ParseUserPreferences(contentRestrictionLevel))
        .SetUserClassification(userClassification)
        .SetParameters(parameters)
        .SetRequestSource(requestSource)
        .SetIotUserInfo(iotUserInfo)
        .SetContactsList(contactsList)
        .SetOrigin(origin)
        .SetWhisperInfo(whisperInfo)
        .SetCallbackOwnerScenario(callbackOwnerScenario)
        .SetIsWarmUp(isWarmUp)
        .SetAllParsedSemanticFrames(allParsedSemanticFrames)
        .SetDisableVoiceSession(disableVoiceSession)
        .SetDisableShouldListen(disableShouldListen)
        .SetGuestOptions(guestOptions);
    if (rawRequest->HasGuestUserData()) {
        builder.SetGuestData(rawRequest->GetGuestUserData());
    }
    return builder;
}

} // namespace

TMaybe<TUserLocation> ParseUserLocation(const NGeobase::TLookup& geoBase, const TMaybe<TRequest::TLocation>& location,
                                        const TString& timezone, TRTLogger& logger) {
    if (location.Empty()) {
        if (timezone.Empty()) {
            return Nothing();
        }
        return TUserLocation(timezone, DEFAULT_USER_TLD);
    }

    auto regionId = geoBase.GetRegionIdByLocation(location->GetLatitude(), location->GetLongitude());
    TString realTimezone;
    try {
        realTimezone = geoBase.GetTimezoneByLocation(location->GetLatitude(), location->GetLongitude()).Name;
    } catch (...) {
        LOG_ERR(logger) << "Cannot get timezone from geoBase. Using skr->application timezone instead";
        realTimezone = timezone;
    }
    auto cityRegion = NGeobase::UNKNOWN_REGION;
    if (IsValidId(regionId)) {
        cityRegion = geoBase.GetParentIdWithType(regionId, static_cast<int>(NGeobase::ERegionType::CITY));
    }

    // may be geo id is bigger then city or don't have city parent
    NGeobase::TId userRegion = IsValidId(cityRegion) ? cityRegion : regionId;
    return TUserLocation(geoBase, userRegion, realTimezone);
}

TRequest::TInterfaces ParseInterfaces(const TExperimentsProto& experiments,
                                      const google::protobuf::RepeatedPtrField<TProtoStringType>& supported,
                                      const google::protobuf::RepeatedPtrField<TProtoStringType>& unsupported,
                                      const TClientInfoProto& application,
                                      bool isTvPluggedIn, bool voiceSession) {
    TVector<TStringBuf> expFlags(Reserve(experiments.GetStorage().size()));
    for (const auto& expFlag : experiments.GetStorage()) {
        expFlags.emplace_back(expFlag.first);
    }

    const TClientFeatures clientFeatures{application, supported, unsupported, expFlags};

    TRequest::TInterfaces interfaces{};
    // interfaces should rely on supported/unsupported features
    interfaces.SetHasScreen(clientFeatures.HasScreen());
    interfaces.SetIsTvPlugged(isTvPluggedIn);
    interfaces.SetVoiceSession(voiceSession);
    // unsupported features should be used to indicate lack of ability
    interfaces.SetHasReliableSpeakers(!clientFeatures.SupportsNoReliableSpeakers());
    interfaces.SetHasBluetooth(!clientFeatures.SupportsNoBluetooth());
    interfaces.SetHasMicrophone(!clientFeatures.SupportsNoMicrophone());
    // trusted interfaces
    interfaces.SetAudioCodecAAC(clientFeatures.SupportsAudioCodecAAC());
    interfaces.SetAudioCodecAC3(clientFeatures.SupportsAudioCodecAC3());
    interfaces.SetAudioCodecEAC3(clientFeatures.SupportsAudioCodecEAC3());
    interfaces.SetAudioCodecOPUS(clientFeatures.SupportsAudioCodecOPUS());
    interfaces.SetAudioCodecVORBIS(clientFeatures.SupportsAudioCodecVORBIS());
    interfaces.SetCanChangeAlarmSound(clientFeatures.SupportsChangeAlarmSound());
    interfaces.SetCanChangeAlarmSoundLevel(clientFeatures.SupportsChangeAlarmSoundLevel());
    interfaces.SetCanHandleAndroidAppIntent(clientFeatures.SupportsHandleAndroidAppIntent());
    interfaces.SetCanOpenBonusCardsCamera(clientFeatures.SupportsBonusCardsCamera());
    interfaces.SetCanOpenBonusCardsList(clientFeatures.SupportsBonusCardsList());
    interfaces.SetCanOpenCovidQrCode(clientFeatures.SupportsCovidQrCodeLink());
    interfaces.SetCanOpenDialogsInTabs(clientFeatures.SupportsMultiTabs());
    interfaces.SetCanOpenIBroSettings(clientFeatures.SupportsOpenIBroSettings());
    interfaces.SetCanOpenKeyboard(clientFeatures.SupportsOpenKeyboard());
    interfaces.SetCanOpenLink(clientFeatures.SupportsOpenLink());
    interfaces.SetCanOpenLinkIntent(clientFeatures.SupportsIntentUrls());
    interfaces.SetCanOpenLinkSearchViewport(clientFeatures.SupportsOpenLinkSearchViewport());
    interfaces.SetCanOpenLinkTurboApp(clientFeatures.SupportsOpenLinkTurboApp());
    interfaces.SetCanOpenLinkYellowskin(clientFeatures.SupportsOpenLinkYellowskin());
    interfaces.SetCanOpenPasswordManager(clientFeatures.SupportsOpenPasswordManager());
    interfaces.SetCanOpenPedometer(clientFeatures.SupportsPedometer());
    interfaces.SetCanOpenQuasarScreen(clientFeatures.SupportsOpenQuasarScreen());
    interfaces.SetSupportsTandemSetup(clientFeatures.SupportsTandemSetup());
    interfaces.SetSupportsIotIosDeviceSetup(clientFeatures.SupportsIotIosDeviceSetup());
    interfaces.SetSupportsIotAndroidDeviceSetup(clientFeatures.SupportsIotAndroidDeviceSetup());
    interfaces.SetCanOpenReader(clientFeatures.SupportsReader());
    interfaces.SetCanOpenVideoEditor(clientFeatures.SupportsOpenVideoEditor());
    interfaces.SetCanOpenVideotranslationOnboarding(clientFeatures.SupportsVideoTranslationOnboarding());
    interfaces.SetCanOpenWhocalls(clientFeatures.SupportsOpenWhocalls());
    interfaces.SetCanOpenWhocallsBlocking(clientFeatures.SupportsOpenWhocallsBlocking());
    interfaces.SetCanOpenWhocallsMessageFiltering(clientFeatures.SupportsOpenWhocallsMessageFiltering());
    interfaces.SetCanOpenYandexAuth(clientFeatures.SupportsOpenYandexAuth());
    interfaces.SetCanReadSites(clientFeatures.SupportsReadSites());
    interfaces.SetCanRecognizeImage(clientFeatures.SupportsImageRecognizer());
    interfaces.SetCanRecognizeMusic(clientFeatures.SupportsMusicRecognizer());
    interfaces.SetCanRenderDiv2Cards(clientFeatures.SupportsDiv2Cards());
    interfaces.SetCanRenderDivCards(clientFeatures.SupportsDivCards());
    interfaces.SetCanServerAction(clientFeatures.SupportsServerAction());
    interfaces.SetCanSetAlarm(clientFeatures.SupportsAlarms());
    interfaces.SetCanSetAlarmSemanticFrame(clientFeatures.SupportsSemanticFrameAlarms());
    interfaces.SetCanSetTimer(clientFeatures.SupportsTimers());
    interfaces.SetCanShowGif(clientFeatures.SupportsGif());
    interfaces.SetCanShowTimer(clientFeatures.SupportsTimersShowResponse());
    interfaces.SetDynamicRangeDV(clientFeatures.SupportsDynamicRangeDV());
    interfaces.SetDynamicRangeHDR10(clientFeatures.SupportsDynamicRangeHDR10());
    interfaces.SetDynamicRangeHDR10Plus(clientFeatures.SupportsDynamicRangeHDR10Plus());
    interfaces.SetDynamicRangeHLG(clientFeatures.SupportsDynamicRangeHLG());
    interfaces.SetDynamicRangeSDR(clientFeatures.SupportsDynamicRangeSDR());
    interfaces.SetHasAccessToBatteryPowerState(clientFeatures.SupportsBatteryPowerState());
    interfaces.SetHasAudioClient(clientFeatures.SupportsAudioClient());
    interfaces.SetHasAudioClientHls(clientFeatures.SupportsAudioClientHls());
    interfaces.SetHasAudioClientHlsMultiroom(clientFeatures.SupportsAudioClientHlsMultiroom());
    interfaces.SetHasBluetoothPlayer(clientFeatures.SupportsBluetoothPlayer());
    interfaces.SetHasCEC(clientFeatures.SupportsCecAvailable());
    interfaces.SetHasClockDisplay(clientFeatures.SupportsClockDisplay());
    interfaces.SetHasCloudPush(clientFeatures.SupportsCloudPush());
    interfaces.SetHasDirectiveSequencer(clientFeatures.SupportsDirectiveSequencer());
    interfaces.SetHasEqualizer(clientFeatures.SupportsEqualizer());
    interfaces.SetHasLedDisplay(clientFeatures.SupportsLedDisplay());
    interfaces.SetHasMordoviaWebView(clientFeatures.SupportsMordoviaWebview());
    interfaces.SetHasMusicPlayer(clientFeatures.SupportsMusicPlayer());
    interfaces.SetHasMusicPlayerShots(clientFeatures.SupportsMusicPlayerAllowShots());
    interfaces.SetHasMusicQuasarClient(clientFeatures.SupportsMusicQuasarClient());
    interfaces.SetHasMusicSdkClient(clientFeatures.SupportsMusicSDKPlayer());
    interfaces.SetHasNavigator(clientFeatures.SupportsNavigator());
    interfaces.SetHasNotifications(clientFeatures.SupportsNotifications());
    interfaces.SetHasScledDisplay(clientFeatures.SupportsScledDisplay());
    interfaces.SetHasSynchronizedPush(clientFeatures.SupportsSynchronizedPush());
    interfaces.SetHasTvStore(clientFeatures.SupportsTvOpenStore());
    interfaces.SetIncomingMessengerCalls(clientFeatures.SupportsIncomingMessengerCalls());
    interfaces.SetIsPubliclyAvailable(clientFeatures.SupportsPublicAvailability());
    interfaces.SetLiveTvScheme(clientFeatures.SupportsLiveTvScheme());
    interfaces.SetMultiroom(clientFeatures.SupportsMultiroom());
    interfaces.SetMultiroomAudioClient(clientFeatures.SupportsMultiroomAudioClient());
    interfaces.SetMultiroomCluster(clientFeatures.SupportsMultiroomCluster());
    interfaces.SetOpenAddressBook(clientFeatures.SupportsOpenAddressBook());
    interfaces.SetOutgoingPhoneCalls(clientFeatures.SupportsPhoneCalls());
    interfaces.SetPhoneAddressBook(clientFeatures.SupportsPhoneAddressBook());
    interfaces.SetSupportsAbsoluteVolumeChange(clientFeatures.SupportsAbsoluteVolumeChange());
    interfaces.SetSupportsAnyPlayer(clientFeatures.SupportsAnyPlayer());
    interfaces.SetSupportsAudioBitrate192Kbps(clientFeatures.SupportsAudioBitrate192Kbps());
    interfaces.SetSupportsAudioBitrate320Kbps(clientFeatures.SupportsAudioBitrate320Kbps());
    interfaces.SetSupportsBluetoothRCU(clientFeatures.SupportsBluetoothRCU());
    interfaces.SetSupportsButtons(clientFeatures.SupportsButtons());
    interfaces.SetSupportsCloudUi(clientFeatures.SupportsCloudUi());
    interfaces.SetSupportsCloudUiFilling(clientFeatures.SupportsCloudUiFilling());
    interfaces.SetSupportsContentChannelAlarm(clientFeatures.SupportsContentChannelAlarm());
    interfaces.SetSupportsDarkTheme(clientFeatures.SupportsDarkTheme());
    interfaces.SetSupportsDarkThemeSetting(clientFeatures.SupportsDarkThemeSetting());
    interfaces.SetSupportsDivCardsRendering(clientFeatures.SupportsDivCardsRendering());
    interfaces.SetSupportsDoNotDisturbDirective(clientFeatures.SupportsDoNotDisturbDirective());
    interfaces.SetSupportsFeedback(clientFeatures.SupportsFeedback());
    interfaces.SetSupportsFMRadio(clientFeatures.SupportsFMRadio());
    interfaces.SetSupportsGoHomeDirective(clientFeatures.SupportsGoHomeDirective());
    interfaces.SetSupportsHDMIOutput(clientFeatures.SupportsHDMIOutput());
    interfaces.SetSupportsMapsDownloadOffline(clientFeatures.SupportsMapsDownloadOffline());
    interfaces.SetSupportsMuteUnmuteVolume(clientFeatures.SupportsMuteUnmuteVolume());
    interfaces.SetSupportsOpenLinkOutgoingDeviceCalls(clientFeatures.SupportsOpenLinkOutgoingDeviceCalls());
    interfaces.SetSupportsOutgoingDeviceCalls(clientFeatures.SupportsOutgoingDeviceCalls());
    interfaces.SetSupportsOutgoingOperatorCalls(clientFeatures.SupportsOutgoingOperatorCalls());
    interfaces.SetSupportsPlayerContinueDirective(clientFeatures.SupportsPlayerContinueDirective());
    interfaces.SetSupportsPlayerDislikeDirective(clientFeatures.SupportsPlayerDislikeDirective());
    interfaces.SetSupportsPlayerLikeDirective(clientFeatures.SupportsPlayerLikeDirective());
    interfaces.SetSupportsPlayerNextTrackDirective(clientFeatures.SupportsPlayerNextTrackDirective());
    interfaces.SetSupportsPlayerPauseDirective(clientFeatures.SupportsPlayerPauseDirective());
    interfaces.SetSupportsPlayerPreviousTrackDirective(clientFeatures.SupportsPlayerPreviousTrackDirective());
    interfaces.SetSupportsPlayerRewindDirective(clientFeatures.SupportsPlayerRewindDirective());
    interfaces.SetSupportsRelativeVolumeChange(clientFeatures.SupportsRelativeVolumeChange());
    interfaces.SetSupportsRouteManagerCapability(clientFeatures.SupportsRouteManagerCapability());
    interfaces.SetSupportsS3Animations(clientFeatures.SupportsS3Animations());
    interfaces.SetSupportsShowPromo(clientFeatures.SupportsShowPromo());
    interfaces.SetSupportsShowView(clientFeatures.SupportsShowView());
    interfaces.SetSupportsShowViewLayerContent(clientFeatures.SupportsShowViewLayerContent());
    interfaces.SetSupportsShowViewLayerFooter(clientFeatures.SupportsShowViewLayerFooter());
    interfaces.SetSupportsTvOpenCollectionScreenDirective(clientFeatures.SupportsTvOpenCollectionScreenDirective());
    interfaces.SetSupportsTvOpenDetailsScreenDirective(clientFeatures.SupportsTvOpenDetailsScreenDirective());
    interfaces.SetSupportsTvOpenPersonScreenDirective(clientFeatures.SupportsTvOpenPersonScreenDirective());
    interfaces.SetSupportsTvOpenSearchScreenDirective(clientFeatures.SupportsTvOpenSearchScreenDirective());
    interfaces.SetSupportsTvOpenSeriesScreenDirective(clientFeatures.SupportsTvOpenSeriesScreenDirective());
    interfaces.SetSupportsDeviceLocalReminders(clientFeatures.SupportsDeviceLocalReminders());
    interfaces.SetSupportsUnauthorizedMusicDirectives(clientFeatures.SupportsUnauthorizedMusicDirectives());
    interfaces.SetSupportsVerticalScreenNavigation(clientFeatures.SupportsVerticalScreenNavigation());
    interfaces.SetSupportsVideoPlayDirective(clientFeatures.SupportsVideoPlayDirective());
    interfaces.SetSupportsVideoPlayer(clientFeatures.SupportsVideoPlayer());
    interfaces.SetSupportsVideoProtocol(clientFeatures.SupportsVideoProtocol());
    interfaces.SetTtsPlayPlaceholder(clientFeatures.SupportsTtsPlayPlaceholder());
    interfaces.SetVideoCodecAVC(clientFeatures.SupportsVideoCodecAVC());
    interfaces.SetVideoCodecHEVC(clientFeatures.SupportsVideoCodecHEVC());
    interfaces.SetVideoCodecVP9(clientFeatures.SupportsVideoCodecVP9());
    interfaces.SetSupportsPhoneAssistant(clientFeatures.SupportsPhoneAssistant());

    return interfaces;
}

bool IsProtoVinsEnabled(const TSpeechKitRequest& request) {
    return !request.HasExpFlag(EXP_DISABLE_PROTO_VINS_SCENARIO);
}

TMaybe<TRequest::TLocation> ParseLocation(const TMaybe<NAlice::TLocation>& location,
                                          const TMaybe<google::protobuf::Struct>& laasRegion) {
    const auto laas = TryParseLaasLocation(laasRegion);

    if (laas.Defined()) {
        if (!location.Defined() || laas->GetAccuracy() < location->GetAccuracy()) {
            return laas;
        }
    }
    if (location.Defined()) {
        return TRequest::TLocation{location->GetLat(), location->GetLon(), location->GetAccuracy(),
                                   location->GetRecency(), location->GetSpeed()};
    }
    return Nothing();
}

TRequest CreateRequest(std::unique_ptr<const IEvent> event, const TSpeechKitRequest& rawRequest,
                       const TMaybe<TIoTUserInfo>& iotUserInfo,
                       NScenarios::TScenarioBaseRequest::ERequestSourceType requestSource,
                       const TVector<TSemanticFrame>& semanticFrames,
                       const TVector<TSemanticFrame>& recognizedActionEffectFrames,
                       const TStackEngineCore& stackEngineCore, const TRequest::TParameters& parameters,
                       const TMaybe<NAlice::NData::TContactsList>& contactsList, const TMaybe<TOrigin>& origin,
                       ui64 lastWhisperTimeMs, ui64 whisperTtlMs, const TMaybe<TString>& callbackOwnerScenario,
                       const TMaybe<TRequest::TTtsWhisperConfig>& whisperConfig, TRTLogger& logger, bool isWarmUp,
                       const TVector<TSemanticFrame>& allParsedSemanticFrames,
                       const TMaybe<bool> disableVoiceSession, const TMaybe<bool> disableShouldListen) {
    return CreateRequestBuilder(std::move(event), rawRequest, semanticFrames, recognizedActionEffectFrames,
                                stackEngineCore, parameters, requestSource, iotUserInfo, contactsList, origin,
                                lastWhisperTimeMs, whisperTtlMs, callbackOwnerScenario, whisperConfig, logger, isWarmUp, allParsedSemanticFrames,
                                disableVoiceSession.Defined() ? *disableVoiceSession : false,
                                disableShouldListen.Defined() ? *disableShouldListen : false)
        .Build();
}

TRequest CreateRequest(std::unique_ptr<const IEvent> event, const TSpeechKitRequest& rawRequest,
                       const NGeobase::TLookup& geoBase,
                       const TMaybe<TIoTUserInfo>& iotUserInfo,
                       NScenarios::TScenarioBaseRequest::ERequestSourceType requestSource,
                       const TVector<TSemanticFrame>& semanticFrames,
                       const TVector<TSemanticFrame>& recognizedActionEffectFrames,
                       const TStackEngineCore& stackEngineCore, const TRequest::TParameters& parameters,
                       const TMaybe<NAlice::NData::TContactsList>& contactsList,
                       const TMaybe<TOrigin>& origin, ui64 lastWhisperTimeMs, ui64 whisperTtlMs, const TMaybe<TString>& callbackOwnerScenario,
                       const TMaybe<TRequest::TTtsWhisperConfig>& whisperConfig, TRTLogger& logger, bool isWarmUp,
                       const TVector<TSemanticFrame>& allParsedSemanticFrames,
                       const TMaybe<bool> disableVoiceSession, const TMaybe<bool> disableShouldListen) {
    auto builder = CreateRequestBuilder(std::move(event), rawRequest, semanticFrames, recognizedActionEffectFrames,
                                        stackEngineCore, parameters, requestSource, iotUserInfo, contactsList, origin,
                                        lastWhisperTimeMs, whisperTtlMs, callbackOwnerScenario, whisperConfig, logger, isWarmUp, allParsedSemanticFrames,
                                        disableVoiceSession.Defined() ? *disableVoiceSession : false,
                                        disableShouldListen.Defined() ? *disableShouldListen : false);
    builder.SetUserLocation(
        ParseUserLocation(geoBase, builder.GetLocation(), rawRequest->GetApplication().GetTimezone(), logger)
            .GetOrElse(DEFAULT_USER_LOCATION));
    return std::move(builder).Build();
}

TRequest CreateRequest(const NMegamind::TRequestData& requestData) {
    auto event = IEvent::CreateEvent(requestData.GetEvent());
    Y_ENSURE(event);

    TRequestBuilder builder{std::move(event)};

    if (const auto& dialogId = requestData.GetDialogId(); !dialogId.empty()) {
        builder.SetDialogId(dialogId);
    }
    if (const auto& scenarioDialogId = requestData.GetScenarioDialogId(); !scenarioDialogId.empty()) {
        builder.SetScenarioDialogId(scenarioDialogId);
    }

    if (requestData.HasScenario()) {
        EScenarioNameSource source;
        switch (requestData.GetScenario().GetSource()) {
            case NMegamind::TRequestData_TScenarioInfo_EScenarioNameSource_DialogId:
                source = EScenarioNameSource::DialogId;
                break;
            case NMegamind::TRequestData_TScenarioInfo_EScenarioNameSource_ServerAction:
                source = EScenarioNameSource::ServerAction;
                break;
            case NMegamind::TRequestData_TScenarioInfo_EScenarioNameSource_VinsDialogId:
                source = EScenarioNameSource::VinsDialogId;
                break;
            case NMegamind::TRequestData_TScenarioInfo_EScenarioNameSource_VinsServerAction:
                source = EScenarioNameSource::VinsServerAction;
                break;
            default:
                Y_UNREACHABLE();
        }
        builder.SetScenario(TRequest::TScenarioInfo{requestData.GetScenario().GetName(), source});
    }

    if (requestData.HasLocation()) {
        const auto& requestLocation = requestData.GetLocation();
        builder.SetLocation(TRequest::TLocation{requestLocation.GetLatitude(),
                                                requestLocation.GetLongitude(),
                                                requestLocation.GetAccuracy(),
                                                requestLocation.GetRecency(),
                                                requestLocation.GetSpeed()});
    }

    const auto& userLocation = requestData.GetUserLocation();
    builder.SetUserLocation(TUserLocation{userLocation.GetUserTld(),
                                          userLocation.GetUserRegion(),
                                          userLocation.GetUserTimeZone(),
                                          userLocation.GetUserCountry()});

    EContentRestrictionLevel contentRestrictionLevel;
    switch (requestData.GetContentRestrictionLevel()) {
        case NMegamind::TRequestData_EContentRestrictionLevel_Medium:
            contentRestrictionLevel = EContentRestrictionLevel::Medium;
            break;
        case NMegamind::TRequestData_EContentRestrictionLevel_Children:
            contentRestrictionLevel = EContentRestrictionLevel::Children;
            break;
        case NMegamind::TRequestData_EContentRestrictionLevel_Without:
            contentRestrictionLevel = EContentRestrictionLevel::Without;
            break;
        case NMegamind::TRequestData_EContentRestrictionLevel_Safe:
            contentRestrictionLevel = EContentRestrictionLevel::Safe;
            break;
        default:
            Y_UNREACHABLE();
    }
    builder.SetContentRestrictionLevel(contentRestrictionLevel);

    const auto& requestSemanticFrames = requestData.GetSemanticFrames();
    builder.SetSemanticFrames({requestSemanticFrames.begin(), requestSemanticFrames.end()});

    if (!requestData.GetRecognizedActionEffectFrames().empty()) {
        const auto& frames = requestData.GetRecognizedActionEffectFrames();
        builder.SetRecognizedActionEffectFrames({frames.begin(), frames.end()});
    }

    builder.SetStackEngineCore(requestData.GetStackEngineCore());
    builder.SetServerTimeMs(requestData.GetServerTimeMs());
    builder.SetInterfaces(requestData.GetInterfaces());
    builder.SetOptions(requestData.GetOptions());
    builder.SetUserPreferences(requestData.GetUserPreferences());
    builder.SetUserClassification(requestData.GetUserClassification());

    if (requestData.HasParameters()) {
        const auto& parameters = requestData.GetParameters();
        builder.SetParameters(TRequest::TParameters{
            parameters.HasForcedShouldListen()
                ? TMaybe<bool>(parameters.GetForcedShouldListen())
                : Nothing(),
            parameters.HasChannel()
                ? TMaybe<TDirectiveChannel::EDirectiveChannel>(parameters.GetChannel())
                : Nothing(),
            parameters.HasForcedEmotion()
                ? TMaybe<TString>(parameters.GetForcedEmotion())
                : Nothing()});
    }

    builder.SetRequestSource(requestData.GetRequestSource());

    if (requestData.HasIoTUserInfo()) {
        builder.SetIotUserInfo(requestData.GetIoTUserInfo());
    }

    if (requestData.HasContactsList()) {
        builder.SetContactsList(requestData.GetContactsList());
    }

    if (requestData.HasOrigin()) {
        builder.SetOrigin(requestData.GetOrigin());
    }

    if (requestData.HasWhisperInfo()) {
        const auto& whisperInfoProto = requestData.GetWhisperInfo();
        TMaybe<TRequest::TTtsWhisperConfig> whisperConfig;
        if (whisperInfoProto.HasWhisperConfig()) {
            whisperConfig = whisperInfoProto.GetWhisperConfig();
        }
        builder.SetWhisperInfo(
            TRequest::TWhisperInfo{whisperInfoProto.GetIsVoiceInput(), whisperInfoProto.GetLastWhisperTimeMs(),
                                   whisperInfoProto.GetServerTimeMs(), whisperInfoProto.GetWhisperTtlMs(),
                                   whisperInfoProto.GetIsAsrWhisper(), whisperConfig});
    }

    if (requestData.HasCallbackOwnerScenario()) {
        builder.SetCallbackOwnerScenario(requestData.GetCallbackOwnerScenario());
    }

    builder.SetIsWarmUp(requestData.GetIsWarmUp());

    const auto& allParsedSemanticFrames = requestData.GetAllParsedSemanticFrames();
    builder.SetAllParsedSemanticFrames({allParsedSemanticFrames.begin(), allParsedSemanticFrames.end()});
    if (requestData.HasGuestData()) {
        builder.SetGuestData(requestData.GetGuestData());
    }

    if (requestData.HasGuestOptions()) {
        builder.SetGuestOptions(requestData.GetGuestOptions());
    }

    builder.SetDisableVoiceSession(requestData.GetDisableVoiceSession());
    builder.SetDisableShouldListen(requestData.GetDisableShouldListen());

    return std::move(builder).Build();
}

} // namespace NMegamind

} // namespace NAlice

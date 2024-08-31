#pragma once

#include <alice/megamind/library/request/event/event.h>
#include <alice/megamind/library/request/internal/protos/request_data.pb.h>
#include <alice/megamind/library/speechkit/request.h>
#include <alice/megamind/library/stack_engine/protos/stack_engine.pb.h>
#include <alice/megamind/protos/common/directive_channel.pb.h>
#include <alice/megamind/protos/common/frame.pb.h>
#include <alice/megamind/protos/common/iot.pb.h>
#include <alice/megamind/protos/common/origin.pb.h>
#include <alice/megamind/protos/guest/guest_data.pb.h>
#include <alice/megamind/protos/guest/guest_options.pb.h>
#include <alice/megamind/protos/scenarios/request.pb.h>

#include <alice/library/geo/user_location.h>
#include <alice/library/logger/logger.h>
#include <alice/library/restriction_level/restriction_level.h>

#include <alice/memento/proto/user_configs.pb.h>

#include <alice/protos/data/contacts.pb.h>

#include <util/generic/maybe.h>
#include <util/generic/string.h>

#include <memory>

namespace NAlice {

enum class EScenarioNameSource {
    DialogId /* "dialog_id" */,
    ServerAction /* "server_action" */,
    /**
     * Case when no scenario name in DialogId provided
     */
    VinsDialogId /* "vins_dialog_id" */,
    /**
     * Case when no scenario specific fields in ServerAction provided
     */
    VinsServerAction /* "vins_server_action" */
};

class TRequest final {
public:
    using TTtsWhisperConfig = ru::yandex::alice::memento::proto::TTtsWhisperConfig;

    class TScenarioInfo final {
    public:
        TScenarioInfo(const TString& name, EScenarioNameSource source);

        [[nodiscard]] const TString& GetName() const;

        [[nodiscard]] const EScenarioNameSource& GetSource() const;

    private:
        TString Name;
        EScenarioNameSource Source;
    };

    class TWhisperInfo final {
    public:
        TWhisperInfo(bool isVoiceInput, ui64 lastWhisperTimeMs, ui64 serverTimeMs, ui64 whisperTtlMs,
                     bool isAsrWhisper, const TMaybe<TTtsWhisperConfig>& whisperConfig);

        ui64 GetUpdatedLastWhisperTimeMs() const;
        bool IsWhisper() const;
        bool IsAsrWhisper() const;
        bool IsPreviousRequestWhisper() const;

        NMegamind::TRequestData_TWhisperInfo ToProto() const;

    private:
        bool IsVoiceInput_;
        ui64 LastWhisperTimeMs_;
        ui64 ServerTimeMs_;
        ui64 WhisperTtlMs_;
        bool IsAsrWhisper_;
        TMaybe<TTtsWhisperConfig> WhisperConfig_;
    };

    class TLocation final {
    public:
        TLocation(double latitude, double longitude, double accuracy = 0, double recency = 0, double speed = 0)
            : Latitude(latitude)
            , Longitude(longitude)
            , Accuracy(accuracy)
            , Recency(recency)
            , Speed(speed)
        {
        }

        [[nodiscard]] double GetLatitude() const {
            return Latitude;
        }

        [[nodiscard]] double GetLongitude() const {
            return Longitude;
        }

        [[nodiscard]] double GetAccuracy() const {
            return Accuracy;
        }

        [[nodiscard]] double GetRecency() const {
            return Recency;
        }

        [[nodiscard]] double GetSpeed() const {
            return Speed;
        }

    private:
        double Latitude;
        double Longitude;
        double Accuracy;
        double Recency;
        double Speed;
    };

    class TParameters final {
    public:
        TParameters(const TMaybe<bool>& forcedShouldListen = {},
                    const TMaybe<TDirectiveChannel::EDirectiveChannel> channel = {},
                    const TMaybe<TString> forcedEmotion = {})
            : ForcedShouldListen(forcedShouldListen)
            , Channel(channel)
            , ForcedEmotion(forcedEmotion)
        {
        }

        [[nodiscard]] const TMaybe<bool>& GetForcedShouldListen() const {
            return ForcedShouldListen;
        }

        [[nodiscard]] const TMaybe<TDirectiveChannel::EDirectiveChannel>& GetChannel() const {
            return Channel;
        }

        [[nodiscard]] const TMaybe<TString> GetForcedEmotion() const {
            return ForcedEmotion;
        }

    private:
        TMaybe<bool> ForcedShouldListen;
        TMaybe<TDirectiveChannel::EDirectiveChannel> Channel;
        TMaybe<TString> ForcedEmotion;
    };

    using TInterfaces = NScenarios::TInterfaces;
    using TOptions = NScenarios::TOptions;
    using TUserPreferences = NScenarios::TUserPreferences;
    using TUserClassification = NScenarios::TUserClassification;

public:
    TRequest(std::unique_ptr<const IEvent> event, const TMaybe<TString>& dialogId,
             const TMaybe<TString>& scenarioDialogId, const TMaybe<TScenarioInfo>& scenario,
             const TMaybe<TLocation>& location, const TUserLocation& userLocation,
             EContentRestrictionLevel contentRestrictionLevel, const TVector<TSemanticFrame>& semanticFrames,
             const TVector<TSemanticFrame>& recognizedActionEffectFrames,
             const NMegamind::TStackEngineCore& stackEngineCore, ui64 serverTimeMs, const TInterfaces& interfaces,
             const TOptions& options, const TUserPreferences& userPreferences,
             const TUserClassification& userClassification, const TParameters& parameters,
             NScenarios::TScenarioBaseRequest::ERequestSourceType requestSource,
             const TMaybe<TIoTUserInfo>& iotUserInfo, const TMaybe<NAlice::NData::TContactsList>& contactsList,
             const TMaybe<TOrigin>& origin, const TMaybe<TWhisperInfo>& whisperInfo, const TMaybe<TString>& callbackOwnerScenario,
             bool isWarmUp,
             const TVector<TSemanticFrame>& allParsedSemanticFrames,
             const TMaybe<TGuestData>& guestData, const TMaybe<TGuestOptions>& guestOptions,
             bool disableVoiceSession, bool disableShouldListen)
        : Event(std::move(event))
        , DialogId(dialogId)
        , ScenarioDialogId(scenarioDialogId)
        , Scenario(scenario)
        , Location(location)
        , UserLocation(userLocation)
        , ContentRestrictionLevel(contentRestrictionLevel)
        , SemanticFrames(semanticFrames)
        , RecognizedActionEffectFrames(recognizedActionEffectFrames)
        , StackEngineCore(stackEngineCore)
        , ServerTimeMs(serverTimeMs)
        , Interfaces(interfaces)
        , Options(options)
        , UserPreferences(userPreferences)
        , UserClassification(userClassification)
        , Parameters(parameters)
        , RequestSource(requestSource)
        , IotUserInfo(iotUserInfo)
        , ContactsList(contactsList)
        , Origin(origin)
        , WhisperInfo(whisperInfo)
        , CallbackOwnerScenario(callbackOwnerScenario)
        , IsWarmUp_(isWarmUp)
        , AllParsedSemanticFrames(allParsedSemanticFrames)
        , GuestData(guestData)
        , GuestOptions(guestOptions)
        , DisableVoiceSession(disableVoiceSession)
        , DisableShouldListen(disableShouldListen)
    {
        Y_ENSURE(Event);
    }

    /**
     * DialogId from SpeechKitRequest without any modification
     * @return Original DialogId
     */
    [[nodiscard]] const TMaybe<TString>& GetDialogId() const;

    /**
     * Original DialogId as Megamind got it from scenario, parsed from SpeechKitRequest
     * @return Scenario's DialogId
     */
    [[nodiscard]] const TMaybe<TString>& GetScenarioDialogId() const;

    /**
     * Provide ScenarioInfo if it's possible uniquely identify from the request
     * Sources:
     *   1. DialogId: we add scenario specific prefix, like ("scenario_name:scneario_dialog_id")
     *      If DialogId provided, but no scenario name specified - we decide between Dialogovo and Vins
     *   2. Server action (callback directive): we add scenario specific fields in payload
     *      If it's server action, but no scenario specific field specified - MM_VINS_SCENARIO would be returned
     * MM_VINS_SCENARIO provided for backward compatibility
     * @return ScenarioInfo if exists, Nothing otherwise
     */
    [[nodiscard]] const TMaybe<TScenarioInfo>& GetScenario() const;

    /**
     * GetLocation returns location from SpeechKitRequest
     * Location is determined from two sources: "location" and "laas_region"
     * One that has better accuracy will be returned
     * @return Location if exists, Nothing otherwise
     */
    [[nodiscard]] const TMaybe<TLocation>& GetLocation() const {
        return Location;
    }

    [[nodiscard]] const TUserLocation& GetUserLocation() const {
        return UserLocation;
    }

    /**
     * GetContentRestrictionLevel returns filtration mode from SpeechKitRequest
     * Mode is determined from two sources: "filtration_level" and "content_settings"
     * @return FiltrationMode if exists, default otherwise
     */
    [[nodiscard]] EContentRestrictionLevel GetContentRestrictionLevel() const {
        return ContentRestrictionLevel;
    }

    [[nodiscard]] const IEvent& GetEvent() const {
        Y_ASSERT(Event);
        return *Event;
    }

    [[nodiscard]] const TVector<TSemanticFrame>& GetSemanticFrames() const {
        return SemanticFrames;
    }

    [[nodiscard]] const TVector<TSemanticFrame>& GetRecognizedActionEffectFrames() const {
        return RecognizedActionEffectFrames;
    }

    [[nodiscard]] bool GetIsClassifiedAsChildRequest() const {
        return UserClassification.GetAge() == TRequest::TUserClassification::EAge::TUserClassification_EAge_Child;
    }

    [[nodiscard]] bool GetIsClassifiedAsMaleRequest() const {
        return UserClassification.GetGender() == TRequest::TUserClassification::EGender::TUserClassification_EGender_Male;
    }

    [[nodiscard]] bool GetIsClassifiedAsFemaleRequest() const {
        return UserClassification.GetGender() == TRequest::TUserClassification::EGender::TUserClassification_EGender_Female;
    }

    [[nodiscard]] const NMegamind::TStackEngineCore& GetStackEngineCore() const {
        return StackEngineCore;
    }

    [[nodiscard]] ui64 GetServerTimeMs() const {
        return ServerTimeMs;
    }

    [[nodiscard]] const NScenarios::TInterfaces& GetInterfaces() const {
        return Interfaces;
    }

    [[nodiscard]] const NScenarios::TOptions& GetOptions() const {
        return Options;
    }

    [[nodiscard]] const NScenarios::TUserPreferences& GetUserPreferences() const {
        return UserPreferences;
    }

    [[nodiscard]] const NScenarios::TUserClassification& GetUserClassification() const {
        return UserClassification;
    }

    [[nodiscard]] const TParameters& GetParameters() const {
        return Parameters;
    }

    [[nodiscard]] NScenarios::TScenarioBaseRequest::ERequestSourceType GetRequestSource() const {
        return RequestSource;
    }

    [[nodiscard]] const TMaybe<TIoTUserInfo>& GetIotUserInfo() const {
        return IotUserInfo;
    }

    [[nodiscard]] const TMaybe<NAlice::NData::TContactsList>& GetContactsList() const {
        return ContactsList;
    }

    [[nodiscard]] const TMaybe<TOrigin>& GetOrigin() const {
        return Origin;
    }

    [[nodiscard]] const TMaybe<TWhisperInfo>& GetWhisperInfo() const {
        return WhisperInfo;
    }

    [[nodiscard]] const TMaybe<TString>& GetCallbackOwnerScenario() const {
        return CallbackOwnerScenario;
    }

    [[nodiscard]] bool IsWarmUp() const {
        return IsWarmUp_;
    }

    [[nodiscard]] const TVector<TSemanticFrame>& GetAllParsedSemanticFrames() const {
        return AllParsedSemanticFrames;
    }

    [[nodiscard]] const TMaybe<TGuestData>& GetGuestData() const {
        return GuestData;
    }

    [[nodiscard]] const TMaybe<TGuestOptions>& GetGuestOptions() const {
        return GuestOptions;
    }

    [[nodiscard]] bool GetDisableVoiceSession() const {
        return DisableVoiceSession;
    }

    [[nodiscard]] bool GetDisableShouldListen() const {
        return DisableShouldListen;
    }

    NMegamind::TRequestData ToProto() const;

private:
    std::unique_ptr<const IEvent> Event;

    TMaybe<TString> DialogId;
    TMaybe<TString> ScenarioDialogId;

    TMaybe<TScenarioInfo> Scenario;

    TMaybe<TLocation> Location;
    TUserLocation UserLocation;
    EContentRestrictionLevel ContentRestrictionLevel;
    TVector<TSemanticFrame> SemanticFrames;
    TVector<TSemanticFrame> RecognizedActionEffectFrames;
    NMegamind::TStackEngineCore StackEngineCore;
    ui64 ServerTimeMs;

    TInterfaces Interfaces;
    TOptions Options;
    TUserPreferences UserPreferences;
    TUserClassification UserClassification;
    TParameters Parameters;
    NScenarios::TScenarioBaseRequest::ERequestSourceType RequestSource;

    TMaybe<TIoTUserInfo> IotUserInfo;
    TMaybe<NAlice::NData::TContactsList> ContactsList;
    TMaybe<TOrigin> Origin;
    TMaybe<TWhisperInfo> WhisperInfo;
    TMaybe<TString> CallbackOwnerScenario;
    bool IsWarmUp_;
    TVector<TSemanticFrame> AllParsedSemanticFrames;
    TMaybe<TGuestData> GuestData;
    TMaybe<TGuestOptions> GuestOptions;
    bool DisableVoiceSession;
    bool DisableShouldListen;
};

namespace NMegamind {

inline const TString DEFAULT_USER_TLD = "ru";
inline const auto DEFAULT_USER_LOCATION = TUserLocation(/* userTimezone= */ "Europe/Moscow", DEFAULT_USER_TLD);

// To work properly ProtoVins scenario should be enabled in config
bool IsProtoVinsEnabled(const TSpeechKitRequest& request);

TMaybe<TRequest::TLocation> ParseLocation(const TMaybe<NAlice::TLocation>& location,
                                          const TMaybe<google::protobuf::Struct>& laasRegion);

TMaybe<TUserLocation> ParseUserLocation(const NGeobase::TLookup& geoBase, const TMaybe<TRequest::TLocation>& location,
                                        const TString& timezone, TRTLogger& logger);

TRequest::TInterfaces ParseInterfaces(const TExperimentsProto& experiments,
                                      const google::protobuf::RepeatedPtrField<TProtoStringType>& supported,
                                      const google::protobuf::RepeatedPtrField<TProtoStringType>& unsupported,
                                      const TClientInfoProto& application,
                                      bool isTvPluggedIn, bool voiceSession);

TRequest CreateRequest(std::unique_ptr<const IEvent> event, const TSpeechKitRequest& rawRequest,
                       const TMaybe<TIoTUserInfo>& iotUserInfo = Nothing(),
                       NScenarios::TScenarioBaseRequest::ERequestSourceType requestSource = {},
                       const TVector<TSemanticFrame>& semanticFrames = {},
                       const TVector<TSemanticFrame>& recognizedActionEffectFrames = {},
                       const TStackEngineCore& stackEngineCore = {}, const TRequest::TParameters& parameters = {},
                       const TMaybe<NAlice::NData::TContactsList>& contactsList = Nothing(),
                       const TMaybe<TOrigin>& origin = Nothing(), ui64 lastWhisperTimeMs = 0, ui64 whisperTtlMs = 0,
                       const TMaybe<TString>& callbackOwnerScenario = Nothing(),
                       const TMaybe<TRequest::TTtsWhisperConfig>& whisperConfig = Nothing(),
                       TRTLogger& logger = TRTLogger::NullLogger(), bool isWarmUp = false,
                       const TVector<TSemanticFrame>& allParsedSemanticFrames = {},
                       const TMaybe<bool> disableVoiceSession = {},
                       const TMaybe<bool> disableShouldListen = {});

TRequest CreateRequest(std::unique_ptr<const IEvent> event, const TSpeechKitRequest& rawRequest,
                       const NGeobase::TLookup& geoBase,
                       const TMaybe<TIoTUserInfo>& iotUserInfo = Nothing(),
                       NScenarios::TScenarioBaseRequest::ERequestSourceType requestSource = {},
                       const TVector<TSemanticFrame>& semanticFrames = {},
                       const TVector<TSemanticFrame>& recognizedActionEffectFrame = {},
                       const TStackEngineCore& stackEngineCore = {}, const TRequest::TParameters& parameters = {},
                       const TMaybe<NAlice::NData::TContactsList>& contactsList = Nothing(),
                       const TMaybe<TOrigin>& origin = Nothing(), ui64 lastWhisperTimeMs = 0, ui64 whisperTtlMs = 0,
                       const TMaybe<TString>& callbackOwnerScenario = Nothing(),
                       const TMaybe<TRequest::TTtsWhisperConfig>& whisperConfig = Nothing(),
                       TRTLogger& logger = TRTLogger::NullLogger(), bool isWarmUp = false,
                       const TVector<TSemanticFrame>& allParsedSemanticFrames = {},
                       const TMaybe<bool> disableVoiceSession = {},
                       const TMaybe<bool> disableShouldListen = {});

TRequest CreateRequest(const NMegamind::TRequestData& requestData);

} // namespace NMegamind

} // namespace NAlice

#pragma once

#include "request.h"

#include <alice/megamind/library/stack_engine/protos/stack_engine.pb.h>

#include <util/generic/vector.h>

#include <memory>

namespace NAlice {
namespace NMegamind {

class TRequestBuilder {
public:
    TRequestBuilder(std::unique_ptr<const IEvent> event)
        : Event(std::move(event))
        , UserLocation(DEFAULT_USER_LOCATION)
        , ContentRestrictionLevel(EContentRestrictionLevel::Without)
        , DisableVoiceSession(false)
        , DisableShouldListen(false)
    {
        Y_ENSURE(Event);
    }

    TRequestBuilder(const TRequest& other)
        : Event(IEvent::CreateEvent(other.GetEvent().SpeechKitEvent()))
        , DialogId(other.GetDialogId())
        , ScenarioDialogId(other.GetScenarioDialogId())
        , Scenario(other.GetScenario())
        , Location(other.GetLocation())
        , UserLocation(other.GetUserLocation())
        , ContentRestrictionLevel(other.GetContentRestrictionLevel())
        , SemanticFrames(other.GetSemanticFrames())
        , RecognizedActionEffectFrames(other.GetRecognizedActionEffectFrames())
        , StackEngineCore(other.GetStackEngineCore())
        , ServerTimeMs(other.GetServerTimeMs())
        , Interfaces(other.GetInterfaces())
        , Options(other.GetOptions())
        , UserPreferences(other.GetUserPreferences())
        , UserClassification(other.GetUserClassification())
        , Parameters(other.GetParameters())
        , RequestSource(other.GetRequestSource())
        , IotUserInfo(other.GetIotUserInfo())
        , ContactsList(other.GetContactsList())
        , Origin(other.GetOrigin())
        , WhisperInfo(other.GetWhisperInfo())
        , CallbackOwnerScenario(other.GetCallbackOwnerScenario())
        , IsWarmUp_(other.IsWarmUp())
        , AllParsedSemanticFrames(other.GetAllParsedSemanticFrames())
        , GuestData(other.GetGuestData())
        , GuestOptions(other.GetGuestOptions())
        , DisableVoiceSession(other.GetDisableVoiceSession())
        , DisableShouldListen(other.GetDisableShouldListen())
    {
    }

    TRequestBuilder& SetEvent(std::unique_ptr<const IEvent> event) {
        Y_ENSURE(event);
        Event = std::move(event);
        return *this;
    }

    TRequestBuilder& SetDialogId(const TMaybe<TString>& dialogId) {
        DialogId = dialogId;
        return *this;
    }

    TRequestBuilder& SetScenarioDialogId(const TMaybe<TString>& scenarioDialogId) {
        ScenarioDialogId = scenarioDialogId;
        return *this;
    }

    TRequestBuilder& SetScenario(const TMaybe<TRequest::TScenarioInfo>& scenario) {
        Scenario = scenario;
        return *this;
    }

    TRequestBuilder& SetLocation(const TMaybe<TRequest::TLocation>& location) {
        Location = location;
        return *this;
    }

    TRequestBuilder& SetUserLocation(const TUserLocation& userLocation) {
        UserLocation = userLocation;
        return *this;
    }

    TRequestBuilder& SetContentRestrictionLevel(EContentRestrictionLevel contentRestrictionLevel) {
        ContentRestrictionLevel = contentRestrictionLevel;
        return *this;
    }

    TRequestBuilder& SetSemanticFrames(const TVector<TSemanticFrame>& semanticFrames) {
        SemanticFrames = semanticFrames;
        return *this;
    }

    TRequestBuilder& SetRecognizedActionEffectFrames(const TVector<TSemanticFrame>& recognizedActionEffectFrames) {
        RecognizedActionEffectFrames = recognizedActionEffectFrames;
        return *this;
    }

    TRequestBuilder& SetStackEngineCore(const TStackEngineCore& core) {
        StackEngineCore = core;
        return *this;
    }

    TRequestBuilder& SetServerTimeMs(ui64 serverTimeMs) {
        ServerTimeMs = serverTimeMs;
        return *this;
    }

    TRequestBuilder& SetInterfaces(const TRequest::TInterfaces& interfaces) {
        Interfaces = interfaces;
        return *this;
    }

    TRequestBuilder& SetOptions(const TRequest::TOptions& options) {
        Options = options;
        return *this;
    }

    TRequestBuilder& SetUserPreferences(const TRequest::TUserPreferences& preferences) {
        UserPreferences = preferences;
        return *this;
    }

    TRequestBuilder& SetUserClassification(const NScenarios::TUserClassification& classification) {
        UserClassification = classification;
        return *this;
    }

    TRequestBuilder& SetParameters(const TRequest::TParameters& parameters) {
        Parameters = parameters;
        return *this;
    }

    TRequestBuilder& SetRequestSource(NScenarios::TScenarioBaseRequest::ERequestSourceType requestSource) {
        RequestSource = requestSource;
        return *this;
    }

    TRequestBuilder& SetIotUserInfo(const TMaybe<TIoTUserInfo>& iotUserInfo) {
        IotUserInfo = iotUserInfo;
        return *this;
    }

    TRequestBuilder& SetContactsList(const TMaybe<NAlice::NData::TContactsList>& contactsList) {
        ContactsList = contactsList;
        return *this;
    }

    TRequestBuilder& SetOrigin(const TMaybe<TOrigin>& origin) {
        Origin = origin;
        return *this;
    }

    TRequestBuilder& SetWhisperInfo(const TMaybe<TRequest::TWhisperInfo>& whisperInfo) {
        WhisperInfo = whisperInfo;
        return *this;
    }

    TRequestBuilder& SetCallbackOwnerScenario(const TMaybe<TString>& callbackOwnerScenario) {
        CallbackOwnerScenario = callbackOwnerScenario;
        return *this;
    }

    TRequestBuilder& SetIsWarmUp(bool isWarmUp) {
        IsWarmUp_ = isWarmUp;
        return *this;
    }

    TRequestBuilder& SetGuestData(const TMaybe<TGuestData>& guestData) {
        GuestData = guestData;
        return *this;
    }

    TRequestBuilder& SetGuestOptions(const TMaybe<TGuestOptions>& guestOptions) {
        GuestOptions = guestOptions;
        return *this;
    }

    const TMaybe<TRequest::TLocation>& GetLocation() {
        return Location;
    }

    TRequestBuilder& SetAllParsedSemanticFrames(const TVector<TSemanticFrame>& allParsedSemanticFrames) {
        AllParsedSemanticFrames = allParsedSemanticFrames;
        return *this;
    }

    TRequestBuilder& SetDisableVoiceSession(bool disableVoiceSession) {
        DisableVoiceSession = disableVoiceSession;
        return *this;
    }

    TRequestBuilder& SetDisableShouldListen(bool disableShouldListen) {
        DisableShouldListen = disableShouldListen;
        return *this;
    }

    TRequest Build() && {
        return {std::move(Event),
                DialogId,
                ScenarioDialogId,
                Scenario,
                Location,
                UserLocation,
                ContentRestrictionLevel,
                SemanticFrames,
                RecognizedActionEffectFrames,
                StackEngineCore,
                ServerTimeMs,
                Interfaces,
                Options,
                UserPreferences,
                UserClassification,
                Parameters,
                RequestSource,
                IotUserInfo,
                ContactsList,
                Origin,
                WhisperInfo,
                CallbackOwnerScenario,
                IsWarmUp_,
                AllParsedSemanticFrames,
                GuestData,
                GuestOptions,
                DisableVoiceSession,
                DisableShouldListen};
    }

private:
    std::unique_ptr<const IEvent> Event;

    TMaybe<TString> DialogId;
    TMaybe<TString> ScenarioDialogId;

    TMaybe<TRequest::TScenarioInfo> Scenario;

    TMaybe<TRequest::TLocation> Location;
    TUserLocation UserLocation;
    EContentRestrictionLevel ContentRestrictionLevel;
    TVector<TSemanticFrame> SemanticFrames;
    TVector<TSemanticFrame> RecognizedActionEffectFrames;
    TStackEngineCore StackEngineCore;
    ui64 ServerTimeMs;

    TRequest::TInterfaces Interfaces;
    TRequest::TOptions Options;
    TRequest::TUserPreferences UserPreferences;
    TRequest::TUserClassification UserClassification;
    TRequest::TParameters Parameters;
    NScenarios::TScenarioBaseRequest::ERequestSourceType RequestSource;

    TMaybe<TIoTUserInfo> IotUserInfo;
    TMaybe<NAlice::NData::TContactsList> ContactsList;
    TMaybe<TOrigin> Origin;
    TMaybe<TRequest::TWhisperInfo> WhisperInfo;
    TMaybe<TString> CallbackOwnerScenario;
    bool IsWarmUp_;
    TVector<TSemanticFrame> AllParsedSemanticFrames;
    TMaybe<TGuestData> GuestData;
    TMaybe<TGuestOptions> GuestOptions;
    bool DisableVoiceSession;
    bool DisableShouldListen;
};

} // namespace NMegamind
} // namespace NAlice

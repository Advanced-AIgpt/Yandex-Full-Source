#pragma once

#include "mock_global_context.h"
#include "mock_request_context.h"
#include "speechkit.h"

#include <alice/megamind/library/context/context.h>
#include <alice/megamind/library/context/responses.h>
#include <alice/megamind/library/memento/memento.h>

#include <alice/library/iot/structs.h>
#include <alice/library/geo/user_location.h>
#include <alice/library/metrics/sensors.h>
#include <alice/library/restriction_level/restriction_level.h>

#include <catboost/libs/model/model.h>

#include <library/cpp/testing/gmock_in_unittest/gmock.h>

#include <memory>

namespace NAlice {

class TMockContext : public IContext {
public:
    TMockContext() {
        ON_CALL(*this, ClassificationConfig())
            .WillByDefault(testing::ReturnRef(NMegamind::TClassificationConfig::default_instance()));
        ON_CALL(*this, VinsRequestHintBlurRatio())
            .WillByDefault(testing::Return(0));
        ON_CALL(*this, GetWhisperTtlMs())
            .WillByDefault(testing::Return(123));
        ON_CALL(*this, MementoData())
            .WillByDefault(testing::ReturnRef(MementoData_));
        ON_CALL(*this, GetTtsWhisperConfig())
            .WillByDefault(testing::ReturnRef(TtsWhisperConfig_));
    }

    MOCK_METHOD(THttpHeaders&, HttpHeadersForVinsScenarioOnly, (), (const));
    MOCK_METHOD(TMaybe<TString>, ExpFlag, (TStringBuf), (const, override));
    MOCK_METHOD(bool, HasExpFlag, (TStringBuf), (const, override));
    MOCK_METHOD((const THashMap<TString, TMaybe<TString>>&), ExpFlags, (), (const, override));
    MOCK_METHOD(TSpeechKitRequest, SpeechKitRequest, (), (const, override));
    MOCK_METHOD(TMaybe<TString>, AuthToken, (), (const, override));
    MOCK_METHOD(TMaybe<TString>, AppInfo, (), (const, override));
    MOCK_METHOD(TString, PolyglotUtterance, (), (const, override));
    MOCK_METHOD(TMaybe<TString>, TranslatedUtterance, (), (const, override));
    MOCK_METHOD(TMaybe<TString>, NormalizedTranslatedUtterance, (), (const, override));
    MOCK_METHOD(TMaybe<TString>, NormalizedPolyglotUtterance, (), (const, override));
    MOCK_METHOD(TMaybe<TStringBuf>, AsrNormalizedUtterance, (), (const, override));
    MOCK_METHOD(ELanguage, Language, (), (const, override));
    MOCK_METHOD(ELanguage, LanguageForClassifiers, (), (const, override));

    MOCK_METHOD(const IResponses&, Responses, (), (const, override));
    MOCK_METHOD(bool, HasResponses, (), (const, override));

    MOCK_METHOD(const TUserLocation&, UserLocation, (), (const));

    MOCK_METHOD(const IEvent*, Event, (), (const));

    MOCK_METHOD(TRTLogger&, Logger, (), (const, override));
    MOCK_METHOD(const TClientFeatures&, ClientFeatures, (), (const, override));
    MOCK_METHOD(TClientInfo&, ClientInfo, (), (const, override));
    MOCK_METHOD(const NGeobase::TLookup&, Geobase, (), (const, override));
    MOCK_METHOD(const ISession*, Session, (), (const, override));
    MOCK_METHOD(TSessionProto::TScenarioSession, PreviousScenarioSession, (), (const, override));
    MOCK_METHOD(TStringBuf, BassAvatarsHost, (), (const, override));
    MOCK_METHOD(NMetrics::ISensors&, Sensors, (), (const, override));
    MOCK_METHOD(const TScenarioInfraConfig&, ScenarioConfig, (const TString&), (const, override));
    MOCK_METHOD(const NMegamind::TClassificationConfig&, ClassificationConfig, (), (const, override));
    MOCK_METHOD(bool, IsOAuthEnabled, (const TString&), (const, override));
    MOCK_METHOD(bool, IsDumpRunRequestsModeEnabled, (), (const, override));
    MOCK_METHOD(TString, GetDumpRunRequestsModeOutputDirPath, (), (const, override));
    MOCK_METHOD(TConfig_TDumpRunRequestsMode_EFormat, GetDumpFormat, (), (const, override));
    MOCK_METHOD(bool, IsProtoVinsEnabled, (), (const, override));
    MOCK_METHOD(const NMegamind::TStackEngineCore&, StackEngineCore, (), (const, override));
    MOCK_METHOD(ui64, GetSeed, (), (const, override));
    MOCK_METHOD(IThreadPool&, RequestThreads, (), (const, override));
    MOCK_METHOD(const NMegamind::TMementoData&, MementoData, (), (const, override));
    MOCK_METHOD(const TPartialPreCalcer&, GetPartialPreClassificationCalcer, (), (const, override));
    MOCK_METHOD(bool, HasIoTUserInfo, (), (const, override));
    MOCK_METHOD(const TIoTUserInfo&, IoTUserInfo, (), (const, override));
    MOCK_METHOD(ui32, VinsRequestHintBlurRatio, (), (const, override));
    MOCK_METHOD(ui64, GetWhisperTtlMs, (), (const, override));
    MOCK_METHOD(const NMegamind::NMementoApi::TTtsWhisperConfig&, GetTtsWhisperConfig, (), (const, override));

    void SetResponses(THolder<IResponses> /* responses */) override {
    }

private:
    NMegamind::TMementoData MementoData_;
    NMegamind::NMementoApi::TTtsWhisperConfig TtsWhisperConfig_;
};

class TMockEvent : public IEvent {
public:
    using IEvent::IEvent;

    MOCK_METHOD(const TString&, GetUtterance, (), (const, override));
    MOCK_METHOD(TStringBuf, GetAsrNormalizedUtterance, (), (const, override));
    MOCK_METHOD(bool, HasUtterance, (), (const, override));
    MOCK_METHOD(bool, HasAsrNormalizedUtterance, (), (const, override));
    MOCK_METHOD(void, FillScenarioInput, (const TMaybe<TString>&, NScenarios::TInput*), (const, override));
};

} // namespace NAlice

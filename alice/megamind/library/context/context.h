#pragma once

#include "responses.h"

#include <alice/megamind/library/globalctx/globalctx.h>
#include <alice/megamind/library/memento/memento.h>
#include <alice/megamind/library/request/request.h>
#include <alice/megamind/library/request/event/event.h>
#include <alice/megamind/library/requestctx/fwd.h>
#include <alice/megamind/library/session/session.h>
#include <alice/megamind/library/speechkit/request.h>
#include <alice/megamind/library/walker/source_response_holder.h>

#include <alice/megamind/protos/scenarios/directives.pb.h>
#include <alice/megamind/protos/speechkit/request.pb.h>
#include <alice/megamind/protos/speechkit/response.pb.h>

#include <alice/library/client/client_features.h>
#include <alice/library/geo/user_location.h>
#include <alice/library/metrics/sensors.h>
#include <alice/library/restriction_level/restriction_level.h>
#include <alice/memento/proto/user_configs.pb.h>

#include <library/cpp/langs/langs.h>

#include <util/generic/hash.h>
#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/generic/variant.h>

#include <utility>

namespace NAlice {

using TScenarioInfraConfig = TConfig::TScenarios::TConfig;

class IContext {
public:
    using TExpFlags = NMegamind::TClientComponent::TExpFlags;

public:
    virtual ~IContext() = default;

    virtual TMaybe<TString> AuthToken() const = 0;
    virtual TMaybe<TString> AppInfo() const = 0;

    virtual TRTLogger& Logger() const = 0;
    virtual NMetrics::ISensors& Sensors() const = 0;

    virtual TMaybe<TString> ExpFlag(TStringBuf name) const = 0;
    virtual bool HasExpFlag(TStringBuf name) const = 0;
    virtual const TExpFlags& ExpFlags() const = 0;

    virtual const TClientFeatures& ClientFeatures() const = 0;
    virtual const TClientInfo& ClientInfo() const = 0;
    virtual ui64 GetSeed() const = 0;

    virtual TSpeechKitRequest SpeechKitRequest() const = 0;

    virtual const ISession* Session() const = 0;
    virtual TSessionProto::TScenarioSession PreviousScenarioSession() const = 0;
    // TODO: remove it when only protocol scenarios left
    virtual const NMegamind::TStackEngineCore& StackEngineCore() const = 0;

    virtual TString PolyglotUtterance() const = 0;
    virtual TMaybe<TString> TranslatedUtterance() const = 0;
    virtual TMaybe<TString> NormalizedTranslatedUtterance() const = 0;
    virtual TMaybe<TString> NormalizedPolyglotUtterance() const = 0;
    virtual TMaybe<TStringBuf> AsrNormalizedUtterance() const = 0;
    virtual ELanguage Language() const = 0;
    virtual ELanguage LanguageForClassifiers() const = 0;

    virtual void SetResponses(THolder<IResponses> responses) = 0;
    virtual bool HasResponses() const = 0;
    virtual const IResponses& Responses() const = 0;

    virtual const TScenarioInfraConfig& ScenarioConfig(const TString& scenarioName) const = 0;
    virtual const NMegamind::TClassificationConfig& ClassificationConfig() const = 0;
    virtual bool IsOAuthEnabled(const TString& scenarioName) const = 0;

    virtual bool IsDumpRunRequestsModeEnabled() const = 0;
    virtual TString GetDumpRunRequestsModeOutputDirPath() const = 0;
    virtual TConfig::TDumpRunRequestsMode::EFormat GetDumpFormat() const = 0;
    virtual bool IsProtoVinsEnabled() const = 0;

    virtual const NGeobase::TLookup& Geobase() const = 0;
    virtual TStringBuf BassAvatarsHost() const = 0;

    virtual IThreadPool& RequestThreads() const = 0;

    virtual const NMegamind::TMementoData& MementoData() const = 0;
    virtual const TPartialPreCalcer& GetPartialPreClassificationCalcer() const = 0;

    virtual bool HasIoTUserInfo() const = 0;
    virtual const TIoTUserInfo& IoTUserInfo() const = 0;

    virtual ui32 VinsRequestHintBlurRatio() const = 0;

    virtual ui64 GetWhisperTtlMs() const = 0;

    virtual const NMegamind::NMementoApi::TTtsWhisperConfig& GetTtsWhisperConfig() const = 0;

public:
    // XXX(a-square): A protocol-friendly fix for: https://st.yandex-team.ru/ASSISTANT-3628.
    // Remove this after https://st.yandex-team.ru/IBRO-20637 is resolved.
    // When adding Kazakh scenarios, add the language as well.
    static ELanguage ForceKnownLanguage(TRTLogger& logger, const ELanguage lang, const NMegamind::TClientComponent::TExpFlags& expFlags);
};

class TContext final : public IContext {
public:
    TContext(TSpeechKitRequest skr, THolder<IResponses> responses, TRequestCtx& requestCtx);

    virtual TMaybe<TString> AuthToken() const override;
    virtual TMaybe<TString> AppInfo() const override;

    virtual TRTLogger& Logger() const override;
    virtual NMetrics::ISensors& Sensors() const override;

    virtual TMaybe<TString> ExpFlag(TStringBuf name) const override;
    virtual bool HasExpFlag(TStringBuf name) const override;
    virtual const TExpFlags& ExpFlags() const override;

    virtual const TClientFeatures& ClientFeatures() const override;
    virtual const TClientInfo& ClientInfo() const override;
    virtual ui64 GetSeed() const override;

    virtual TSpeechKitRequest SpeechKitRequest() const override;

    virtual const ISession* Session() const override;
    virtual TSessionProto::TScenarioSession PreviousScenarioSession() const override;
    // TODO: remove it when only protocol scenarios left
    virtual const NMegamind::TStackEngineCore& StackEngineCore() const override;

    virtual TString PolyglotUtterance() const override;
    virtual TMaybe<TString> TranslatedUtterance() const override;
    virtual TMaybe<TString> NormalizedTranslatedUtterance() const override;
    virtual TMaybe<TString> NormalizedPolyglotUtterance() const override;
    virtual TMaybe<TStringBuf> AsrNormalizedUtterance() const override;
    virtual ELanguage Language() const override;
    virtual ELanguage LanguageForClassifiers() const override;

    virtual void SetResponses(THolder<IResponses> responses) override;
    virtual bool HasResponses() const override;
    virtual const IResponses& Responses() const override;

    virtual const TScenarioInfraConfig& ScenarioConfig(const TString& scenarioName) const override;
    const NMegamind::TClassificationConfig& ClassificationConfig() const override;
    virtual bool IsOAuthEnabled(const TString& scenarioName) const override;

    virtual bool IsDumpRunRequestsModeEnabled() const override;
    virtual TString GetDumpRunRequestsModeOutputDirPath() const override;
    virtual TConfig::TDumpRunRequestsMode::EFormat GetDumpFormat() const override;
    virtual bool IsProtoVinsEnabled() const override;

    virtual const NGeobase::TLookup& Geobase() const override;
    virtual TStringBuf BassAvatarsHost() const override;

    virtual IThreadPool& RequestThreads() const override;

    virtual const NMegamind::TMementoData& MementoData() const override;
    virtual const TPartialPreCalcer& GetPartialPreClassificationCalcer() const override;

    virtual bool HasIoTUserInfo() const override;
    virtual const TIoTUserInfo& IoTUserInfo() const override;

    virtual ui32 VinsRequestHintBlurRatio() const override;
    virtual ui64 GetWhisperTtlMs() const override;
    virtual const NMegamind::NMementoApi::TTtsWhisperConfig& GetTtsWhisperConfig() const override;

private:
    void InitSession() const;
    const TConfig& Config() const;
    IGlobalCtx& GlobalCtx() const;
    void ResetSession() const;
    // TODO: remove all event based methods, because this event can be invalidated in walker run
    const IEvent* Event() const;

private:
    const TConfig& Config_;
    const NMegamind::TClassificationConfig& ClassificationConfig_;
    TRequestCtx& RequestCtx_;
    TRTLogger& Logger_;
    TSpeechKitRequest SpeechKitRequest_;

    mutable bool SessionInited_ = false;
    mutable THolder<ISession> Session_;

    ELanguage Language_;
    ELanguage LanguageForClassifiers_;

    THolder<IResponses> Responses_;

    NMegamind::TMementoData MementoData_;

    TMaybe<TIoTUserInfo> IoTUserInfo_;
    NMegamind::NMementoApi::TTtsWhisperConfig DefaultWhisperConfigEnabled;
    NMegamind::NMementoApi::TTtsWhisperConfig DefaultWhisperConfigDisabled;

    mutable NMetrics::TSensorsWrapper RequestSensors_;
};

} // namespace NAlice

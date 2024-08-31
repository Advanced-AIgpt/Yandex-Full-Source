#include "context.h"

#include <alice/megamind/library/experiments/flags.h>
#include <alice/megamind/library/requestctx/requestctx.h>
#include <alice/megamind/library/scenarios/defs/names.h>
#include <alice/megamind/library/serializers/scenario_proto_deserializer.h>
#include <alice/megamind/library/serializers/speechkit_proto_serializer.h>
#include <alice/megamind/library/speechkit/request.h>
#include <alice/megamind/library/speechkit/request_build.h>
#include <alice/megamind/library/worldwide/language/is_alice_worldwide_language.h>

#include <alice/library/experiments/experiments.h>
#include <alice/library/json/json.h>

#include <library/cpp/geobase/lookup.hpp>
#include <library/cpp/protobuf/json/json2proto.h>

#include <util/string/split.h>

namespace NAlice {

namespace {

inline constexpr ui64 DEFAULT_WHISPER_TTL_MS = 6.5 * 60 * 1000;


ELanguage GetLanguageForClassifiers(TRTLogger& logger, const ELanguage requestLanguage, const NMegamind::TClientComponent::TExpFlags& expFlags) {
    if (HasExpFlag(expFlags, EXP_FORCE_RUS_CLASSIFIERS)) {
        LOG_INFO(logger) << "LANG_RUS was forced for classifiers by flag";
        return LANG_RUS;
    }

    return requestLanguage;
}


} // namespace

// IContext --------------------------------------------------------------------
// static
ELanguage IContext::ForceKnownLanguage(TRTLogger& logger, const ELanguage lang, const NMegamind::TClientComponent::TExpFlags& expFlags) {
    if (IsIn({LANG_RUS, LANG_TUR}, lang)) {
        return lang;
    }
    if (lang == LANG_ENG && NAlice::HasExpFlag(expFlags, EXP_ALLOW_LANG_EN)) {
        return lang;
    } else if (lang == LANG_ARA && NAlice::HasExpFlag(expFlags, EXP_ALLOW_LANG_AR)) {
        return lang;
    }
    LOG_WARNING(logger) << "Unexpected language: " << IsoNameByLanguage(lang) << ", defaulting to Russian";
    return LANG_RUS;
}

// TContext --------------------------------------------------------------------
TContext::TContext(TSpeechKitRequest skr, THolder<IResponses> responses, TRequestCtx& requestCtx)
    : Config_{requestCtx.Config()}
    , ClassificationConfig_{requestCtx.ClassificationConfig()}
    , RequestCtx_{requestCtx}
    , Logger_{requestCtx.RTLogger()}
    , SpeechKitRequest_{skr}
    , Language_{ForceKnownLanguage(Logger(), LanguageByName(SpeechKitRequest()->GetApplication().GetLang()), SpeechKitRequest().ExpFlags())}
    , LanguageForClassifiers_(GetLanguageForClassifiers(Logger(), Language(), SpeechKitRequest().ExpFlags()))
    , Responses_{std::move(responses)}
    , RequestSensors_{{{"is_robot", ToString(!skr->GetRequest().GetAdditionalOptions().GetBassOptions().GetProcessId().empty())}}, requestCtx.GlobalCtx().ServiceSensors()}
{
    MementoData_ = GetMementoFromSpeechKitRequest(SpeechKitRequest_, Logger());

    if (SpeechKitRequest_->HasIoTUserInfoData()) {
        try {
            TIoTUserInfo ioTUserInfo;
            ProtoFromBase64String(SpeechKitRequest_->GetIoTUserInfoData(), ioTUserInfo);
            IoTUserInfo_ = std::move(ioTUserInfo);
        } catch(...) {
            LOG_ERR(Logger()) << "Failed to deserialize iot_user_info data: " << CurrentExceptionMessage();
        }
    }
    DefaultWhisperConfigDisabled.SetEnabled(false);
    DefaultWhisperConfigEnabled.SetEnabled(true);
}

TMaybe<TString> TContext::AuthToken() const {
    if (const auto* token = SpeechKitRequest().AuthToken()) {
        return TStringBuilder() << TStringBuf("OAuth ") << *token;
    }
    return Nothing();
}

TMaybe<TString> TContext::AppInfo() const {
    const auto& opts = SpeechKitRequest()->GetRequest().GetAdditionalOptions();
    if (opts.HasAppInfo()) {
        return opts.GetAppInfo();
    }
    return Nothing();
}

TRTLogger& TContext::Logger() const {
    return Logger_;
}

NMetrics::ISensors& TContext::Sensors() const {
    return RequestSensors_;
}

TMaybe<TString> TContext::ExpFlag(TStringBuf name) const {
    return SpeechKitRequest().ExpFlag(name);
}

bool TContext::HasExpFlag(TStringBuf name) const {
    return SpeechKitRequest().HasExpFlag(name);
}

const TContext::TExpFlags& TContext::ExpFlags() const {
    return SpeechKitRequest().ExpFlags();
}

const TClientFeatures& TContext::ClientFeatures() const {
    return SpeechKitRequest().ClientFeatures();
}

const TClientInfo& TContext::ClientInfo() const {
    return SpeechKitRequest().ClientInfo();
}

ui64 TContext::GetSeed() const {
    return SpeechKitRequest().GetSeed();
}

TSpeechKitRequest TContext::SpeechKitRequest() const {
    return SpeechKitRequest_;
}

void TContext::InitSession() const {
    if (Responses_ != nullptr) {
        TStatus error;
        TSessionProto session = Responses_->SpeechKitSessionResponse(&error);
        if (!error) {
            Session_ = MakeMegamindSession(std::move(session));
            SessionInited_ = true;
        }
    }
    if (!SessionInited_) {
        SessionInited_ = true;
        if (SpeechKitRequest()->HasSession()) {
            try {
                Session_ = DeserializeSession(SpeechKitRequest()->GetSession());
                // Remove experiment when all devices have proper reset timeout
                if (SpeechKitRequest().GetResetSession() && HasExpFlag(EXP_ENABLE_SESSION_RESET)) {
                    ResetSession();
                }
            } catch (...) {
                LOG_ERR(Logger()) << "Failed to deserialize session: " << CurrentExceptionMessage();
                Session_.Reset();
            }
        }
    }
}

const ISession* TContext::Session() const {
    if (!SessionInited_) {
        InitSession();
    }
    return Session_.Get();
}

TSessionProto::TScenarioSession TContext::PreviousScenarioSession() const {
    if (!SessionInited_) {
        InitSession();
    }
    return Session_.Get()
        ? Session_->GetPreviousScenarioSession()
        : TSessionProto::TScenarioSession::default_instance();
}

const NMegamind::TStackEngineCore& TContext::StackEngineCore() const {
    if (!SessionInited_) {
        InitSession();
    }
    return Session_ ? Session_->GetStackEngineCore() : NMegamind::TStackEngineCore::default_instance();
}

const IEvent* TContext::Event() const {
    return SpeechKitRequest().EventWrapper().Get();
}

TString TContext::PolyglotUtterance() const {
    if (const auto* event = Event()) {
        return event->GetUtterance();
    }
    return Default<TString>();
}

TMaybe<TString> TContext::TranslatedUtterance() const {
    if (Y_UNLIKELY(!Responses_)) {
        ythrow yexception() << "Responses object is null";
    }
    TStatus error;
    const auto polyglotTranslateUtteranceResponse = Responses_->PolyglotTranslateUtteranceResponse(&error);
    return error ? TMaybe<TString>() : polyglotTranslateUtteranceResponse.GetTranslatedUtterance();
}

TMaybe<TString> TContext::NormalizedPolyglotUtterance() const {
    if (Y_UNLIKELY(!Responses_)) {
        ythrow yexception() << "Responses object is null";
    }
    return Responses_->WizardResponse().GetNormalizedUtterance();
}

TMaybe<TString> TContext::NormalizedTranslatedUtterance() const {
    if (Y_UNLIKELY(!Responses_)) {
        ythrow yexception() << "Responses object is null";
    }
    return Responses_->WizardResponse().GetNormalizedTranslatedUtterance();
}

TMaybe<TStringBuf> TContext::AsrNormalizedUtterance() const {
    if (const auto* event = Event(); event && event->HasAsrNormalizedUtterance()) {
        return event->GetAsrNormalizedUtterance();
    }
    return {};
}

ELanguage TContext::Language() const {
    return Language_;
}

ELanguage TContext::LanguageForClassifiers() const {
    return LanguageForClassifiers_;
}

void TContext::SetResponses(THolder<IResponses> responses) {
    Responses_ = std::move(responses);
}

bool TContext::HasResponses() const {
    return !!Responses_;
}

const IResponses& TContext::Responses() const {
    if (Y_UNLIKELY(!Responses_)) {
        ythrow yexception() << "Responses object is null";
    }
    return *Responses_;
}

const TScenarioInfraConfig& TContext::ScenarioConfig(const TString& scenarioName) const {
    const auto& scenariosConfigs = Config().GetScenarios();
    if (const auto& it = scenariosConfigs.GetConfigs().find(scenarioName); it != scenariosConfigs.GetConfigs().end()) {
        return it->second;
    }
    return scenariosConfigs.GetDefaultConfig();
}

const NMegamind::TClassificationConfig& TContext::ClassificationConfig() const {
    return ClassificationConfig_;
}

bool TContext::IsOAuthEnabled(const TString& scenarioName) const {
    const auto& scenariosConfigs = Config().GetScenarios().GetConfigs();
    if (const auto& it = scenariosConfigs.find(scenarioName); it != scenariosConfigs.end()) {
        return it->second.GetEnableOAuth();
    }
    return false;
}

bool TContext::IsDumpRunRequestsModeEnabled() const {
    return Config().GetDumpRunRequestsMode().GetEnabled();
}

TString TContext::GetDumpRunRequestsModeOutputDirPath() const {
    return Config().GetDumpRunRequestsMode().GetOutputDirPath();
}

TConfig::TDumpRunRequestsMode::EFormat TContext::GetDumpFormat() const {
    return Config().GetDumpRunRequestsMode().GetDumpFormat();
}

bool TContext::IsProtoVinsEnabled() const {
    return NMegamind::IsProtoVinsEnabled(SpeechKitRequest());
}

const NGeobase::TLookup& TContext::Geobase() const {
    return GlobalCtx().GeobaseLookup();
}

TStringBuf TContext::BassAvatarsHost() const {
    return Config().GetBassAvatarsHost();
}

IThreadPool& TContext::RequestThreads() const {
    return GlobalCtx().RequestThreads();
}

const TConfig& TContext::Config() const {
    return Config_;
}

IGlobalCtx& TContext::GlobalCtx() const {
    return RequestCtx_.GlobalCtx();
}

const NMegamind::TMementoData& TContext::MementoData() const {
    return MementoData_;
}

const TPartialPreCalcer& TContext::GetPartialPreClassificationCalcer() const {
    return GlobalCtx().PartialPreClassificationCalcer();
}

void TContext::ResetSession() const {
    if (!SessionInited_) {
        InitSession();
    }
    Session_ = std::move(Session_)->GetUpdater()
        ->SetActions({})
        .SetResponseFrame({})
        .SetResponseEntities({})
        .ModifyScenarioSessions(
            [](const TString& /* scenarioName */, TSessionProto::TScenarioSession& session) {
                session.SetActivityTurn(0);
                session.SetConsequentIrrelevantResponseCount(0);
                session.SetConsequentUntypedSlotRequests(0);
                session.SetTimestamp(TInstant::Now().MicroSeconds());
            })
        .Build();
}

bool TContext::HasIoTUserInfo() const {
    return IoTUserInfo_.Defined();
}

const TIoTUserInfo& TContext::IoTUserInfo() const {
    return *IoTUserInfo_;
}

ui32 TContext::VinsRequestHintBlurRatio() const {
    return Config().GetVinsRequestHintBlurRatio();
}

ui64 TContext::GetWhisperTtlMs() const {
    if (const auto experimentalTtl = GetExperimentValueWithPrefix(ExpFlags(), EXP_MM_WHISPER_TTL_PREFIX);
        experimentalTtl.Defined()) {
        ui64 ttl;
        if (TryFromString<ui64>(*experimentalTtl, ttl)) {
            return ttl;
        }
    }
    return DEFAULT_WHISPER_TTL_MS;
}

const NMegamind::NMementoApi::TTtsWhisperConfig& TContext::GetTtsWhisperConfig() const {
    if (!ClientFeatures().SupportsWhisper()) {
        return DefaultWhisperConfigDisabled;
    }
    const bool isEnabledByDefault = [&] {
        const auto value = GetExperimentValueWithPrefix(ExpFlags(), EXP_DEFAULT_WHISPER_CONFIG_FOR_UNAUTHORIZED_USERS);
        bool result = false;
        if (value.Defined()) {
            TryFromString<bool>(*value, result);
        }
        return result;
    }();
    const auto& defaultConfig = isEnabledByDefault ? DefaultWhisperConfigEnabled : DefaultWhisperConfigDisabled;
    const bool isUserAuthorized = !Responses().BlackBoxResponse().GetUserInfo().GetUid().empty();
    if (isUserAuthorized && (!ClientInfo().ShouldUseDefaultWhisperConfig() ||
                             HasExpFlag(EXP_USE_COMMON_WHISPER_CONFIG_IN_MOBILE_SURFACES))) {
        return MementoData().GetUserConfigs().GetTtsWhisperConfig();
    }
    return defaultConfig;
}

} // namespace NAlice

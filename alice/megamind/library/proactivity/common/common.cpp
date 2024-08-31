#include "common.h"

#include <alice/megamind/library/scenarios/helpers/interface/scenario_ref.h>
#include <alice/megamind/library/scenarios/interface/blackbox.h>
#include <alice/megamind/library/scenarios/interface/scenario.h>

#include <alice/library/network/common.h>
#include <alice/library/proto/protobuf.h>

#include <alice/protos/data/scenario/music/config.pb.h>
#include <dj/services/alisa_skills/server/proto/client/proactivity_request.pb.h>

#include <google/protobuf/util/json_util.h>

#include <library/cpp/langs/langs.h>

#include <util/generic/yexception.h>

namespace NAlice::NMegamind {

namespace {

constexpr TStringBuf SKILL_PROACTIVITY = "SkillProactivity";

constexpr TStringBuf PROACTIVITY_SETTING = "/v1/personality/profile/alisa/kv/alice_proactivity";
constexpr TStringBuf SETTING_ENABLED = "enabled";
constexpr TStringBuf SETTING_DISABLED = "disabled";

constexpr TStringBuf SUBSCRIPTIONS_MANAGER_SCENARIO = "SubscriptionsManager";
constexpr TStringBuf WHAT_CAN_YOU_DO_WITHOUT_SUBSCRIPTION_FRAME = "alice.subscriptions.what_can_you_do_without_subscription";

NDJ::NAS::TProactivityRequest PrepareSkillProactivityRequestBody(const IContext& ctx,
                                                                 const TRequest& requestModel,
                                                                 const TScenarioToRequestFrames& scenarioToFrames,
                                                                 const TProactivityStorage& storage) {
    const auto& skr = ctx.SpeechKitRequest().Proto();

    NDJ::NAS::TProactivityRequest request;
    request.MutableEvent()->CopyFrom(ctx.SpeechKitRequest().Event());
    request.MutableDeviceState()->CopyFrom(skr.GetRequest().GetDeviceState());
    request.MutableClientInfo()->CopyFrom(skr.GetApplication());
    request.MutableUserInfo()->CopyFrom(CreateBlackBoxData(ctx.Responses().BlackBoxResponse()));
    if (ctx.HasIoTUserInfo()) {
        request.MutableIoTUserInfo()->CopyFrom(ctx.IoTUserInfo());
    }
    request.MutableNotificationState()->CopyFrom(skr.GetRequest().GetNotificationState());
    for (const auto&[flag, _] : ctx.ExpFlags()) {
        request.AddExperiments(flag);
    }
    request.SetLanguage(TString{NameByLanguage(ctx.Language())});
    request.MutableLocation()->CopyFrom(requestModel.GetUserLocation().BuildProto());
    request.MutableStorage()->CopyFrom(storage);
    if (const auto* personalIntents = ctx.Responses().PersonalIntentsResponse().ProtoResponse().Get()) {
        request.MutablePersonalIntentsProto()->CopyFrom(*personalIntents);
    }
    request.SetRngSeed(ctx.SpeechKitRequest().GetSeed());
    auto& scenarioMap = *request.MutableScenarioToFrames();
    for (const auto&[scenarioPtr, frames] : scenarioToFrames) {
        if (!scenarioPtr) {
            continue;
        }
        NDJ::NAS::TProactivityRequest::TSemanticFrames scenarioFrames;
        for (const auto& frame : frames) {
            scenarioFrames.AddSemanticFrames()->CopyFrom(frame);
        }
        scenarioMap[scenarioPtr->GetScenario().GetName()] = scenarioFrames;
    }
    request.MutableInterfaces()->CopyFrom(requestModel.GetInterfaces());
    request.MutableOptions()->CopyFrom(requestModel.GetOptions());
    request.MutableUserPreferences()->CopyFrom(requestModel.GetUserPreferences());
    request.MutableUserClassification()->CopyFrom(requestModel.GetUserClassification());

    const auto& whisperInfo = requestModel.GetWhisperInfo();
    if (whisperInfo.Defined()) {
        request.MutableWhisperInfo()->SetIsAsrWhisper(whisperInfo->IsAsrWhisper());
        request.MutableWhisperInfo()->SetIsWhisperResponseAvailable(whisperInfo->IsWhisper());
    }

    auto& mementoData = *request.MutableMementoUserConfigs();
    mementoData.MutableTimeCapsuleInfo()->CopyFrom(ctx.MementoData().GetUserConfigs().GetTimeCapsuleInfo());
    mementoData.MutableMusicConfig()->CopyFrom(ctx.MementoData().GetUserConfigs().GetMusicConfig());

    return request;
}

void MergeFromSession(TProactivityStorage& storage, const TProactivityStorage& session) {
    if (session.GetLastStorageUpdateTime() >= storage.GetLastStorageUpdateTime()) {
        // copy some fields from session
        storage.SetRequestCount(session.GetRequestCount());
    }
}

bool IsSubscriptionsManagerSourceAllowed(const TScenarioToRequestFrames& scenarioToFrames) {
    for (const auto& [scenarioPtr, frames] : scenarioToFrames) {
        if (!scenarioPtr) {
            continue;
        }
        if (scenarioPtr->GetScenario().GetName() == SUBSCRIPTIONS_MANAGER_SCENARIO) {
            return AnyOf(frames, [](const auto& frame){ return frame.GetName() == WHAT_CAN_YOU_DO_WITHOUT_SUBSCRIPTION_FRAME; });
        }
    }
    return false;
}

bool IsWhisperSourceAllowed(const TRequest& requestModel) {
    const auto& whisperInfo = requestModel.GetWhisperInfo();
    return whisperInfo.Defined() && whisperInfo->IsAsrWhisper();
}

} // namespace

TProactivityStorage GetProactivityStorage(const TSpeechKitRequest& skr, const ISession* session,
                                          const TProactivityStorage& mementoProactivityStorage, TRTLogger* logger) {
    const auto tryLogStorage = [logger](TStringBuf name, const TProactivityStorage& storage) {
        if (!logger) {
            return;
        }
        LOG_WITH_TYPE(*logger, TLOG_INFO, ELogMessageType::MegamindPreClasification)
            << SKILL_PROACTIVITY << " " << name << " storage stats:"
            << " LastStorageUpdateTime=" << storage.GetLastStorageUpdateTime()
            << " LastShowTime=" << storage.GetLastShowTime()
            << " PostrollCount=" << storage.GetPostrollCount();
        LOG_WITH_TYPE(*logger, TLOG_DEBUG, ELogMessageType::MegamindPreClasification)
            << SKILL_PROACTIVITY << " " << name << " storage: " << NProtobufJson::Proto2Json(storage);
    };

    TMaybe<TModifiersStorage> modifiersStorage = session ? session->GetModifiersStorage() : TMaybe<TModifiersStorage>{};
    TProactivityStorage* sessionProactivityStoragePtr = modifiersStorage ? modifiersStorage->MutableProactivity() : nullptr;
    if (sessionProactivityStoragePtr) {
        tryLogStorage("session", *sessionProactivityStoragePtr);
    }

    if (!skr.HasExpFlag(EXP_PROACTIVITY_DISABLE_MEMENTO)) {
        tryLogStorage("memento", mementoProactivityStorage);
        TProactivityStorage result = mementoProactivityStorage;
        if (sessionProactivityStoragePtr) {
            MergeFromSession(result, *sessionProactivityStoragePtr);
        }
        return result;
    }

    return sessionProactivityStoragePtr ? std::move(*sessionProactivityStoragePtr) : TProactivityStorage{};
}

TStatus ParseSkillProactivityResponse(const TString& response, NDJ::NAS::TProactivityResponse& outResponse) {
    if (!outResponse.ParseFromString(response)) {
        return TError{TError::EType::Parse} << "Failed to parse proto response";
    }
    return Success();
}

TString GetProactivitySource(const ISession* session, const TScenarioResponse& response) {
    TString source;
    if (const auto& intent = response.GetIntentFromFeatures(); !intent.Empty()) {
        source = intent;
    } else if (session && !session->GetIntentName().Empty()) {
        source = session->GetIntentName();
    } else {
        source = response.GetScenarioName();
    }
    return source;
}

bool IsProactivityDisabledInApp(const TMaybe<NSc::TValue>& personalData) {
    return personalData && personalData.GetRef()[PROACTIVITY_SETTING].GetString(SETTING_ENABLED) == SETTING_DISABLED;
}

TSourcePrepareStatus PrepareSkillProactivityRequest(const IContext& ctx,
                                                    const TRequest& requestModel,
                                                    const TScenarioToRequestFrames& scenarioToFrames,
                                                    const TProactivityStorage& storage,
                                                    NNetwork::IRequestBuilder& request) {
    request.SetContentType(NContentTypes::APPLICATION_PROTOBUF);

    const auto protoBody = PrepareSkillProactivityRequestBody(ctx, requestModel, scenarioToFrames, storage);

    request.SetBody(protoBody.SerializeAsString(), /* method= */ NHttpMethods::POST);

    TCgiParameters cgi;
    // TODO(jan-fazli) Make card_name another parameter, when not only postrolls use this recommender
    cgi.InsertUnescaped(TStringBuf("card_name"), TStringBuf("postroll"));
    cgi.InsertUnescaped(TStringBuf("force_json"), TStringBuf("false"));
    cgi.InsertUnescaped(TStringBuf("msid"), ctx.SpeechKitRequest().RequestId());
    cgi.InsertUnescaped(TStringBuf("client"), TStringBuf("megamind"));
    request.AddCgiParams(cgi);

    LOG_DEBUG(ctx.Logger()) << SKILL_PROACTIVITY << ": Prepared request. Cgi: " << cgi.Print() << "; Post: " << protoBody << Endl;
    return ESourcePrepareType::Succeeded;
}

TProactivityAnswer GetProactivityRecommendations(const NDJ::NAS::TProactivityResponse& response,
                                                 const TString& source, TRTLogger& logger) {
    TProactivityAnswer results;

    LOG_INFO(logger) << SKILL_PROACTIVITY << " Status: " << NDJ::NAS::TProactivityResponse_EStatusCode_Name(response.GetStatus());
    if (response.GetStatus() != NDJ::NAS::TProactivityResponse_EStatusCode_Ok) {
        return results;
    }

    for (const auto& [metaSource, sourceResponse] : response.GetSourceToResponse()) {
        if (source != (sourceResponse.GetSource() ? sourceResponse.GetSource() : metaSource)) {
            continue;
        }
        LOG_INFO(logger) << SKILL_PROACTIVITY << ": Got item " << sourceResponse.GetItemId().Quote() << " for winner source " << source.Quote();
        if (!response.GetItems().count(sourceResponse.GetItemId())) {
            LOG_ERR(logger) << SKILL_PROACTIVITY << ": Selected item is not in response";
            continue;
        }
        NDJ::NAS::TProactivityRecommendation result;
        result.MutableItem()->CopyFrom(response.GetItems().at(sourceResponse.GetItemId()));
        const auto conditionsSize = static_cast<ui64>(response.GetConditions().size());
        for (const auto& conditionIndex : sourceResponse.GetConditionIndices()) {
            if (conditionIndex >= conditionsSize) {
                LOG_ERR(logger) << SKILL_PROACTIVITY << ": Condition " << conditionIndex << " for winner source is not in response";
                continue;
            }
            result.AddConditions()->CopyFrom(response.GetConditions()[conditionIndex]);
        }
        result.MutableContext()->CopyFrom(response.GetContext());
        result.SetScore(sourceResponse.GetScore());
        result.SetQuotaType(response.GetQuotaType());
        LOG_DEBUG(logger) << SKILL_PROACTIVITY << ": Found response for winner source: " << NProtobufJson::Proto2Json(result);
        results.emplace_back(std::move(result));
    }

    if (results.empty()) {
        LOG_INFO(logger) << SKILL_PROACTIVITY << ": No response for winner source " << source.Quote();
    }

    return results;
}

bool ProactivityHasFrequentSources(const TRequest& requestModel,
                                   const TScenarioToRequestFrames& scenarioToFrames) {
    return IsSubscriptionsManagerSourceAllowed(scenarioToFrames) || IsWhisperSourceAllowed(requestModel);

}

} // NAlice::NMegamind

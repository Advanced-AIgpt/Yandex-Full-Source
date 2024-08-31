#pragma once

#include <alice/megamind/library/classifiers/pre.h>
#include <alice/megamind/library/context/context.h>
#include <alice/megamind/library/experiments/flags.h>
#include <alice/megamind/library/request/request.h>
#include <alice/megamind/library/response/response.h>
#include <alice/megamind/library/session/session.h>
#include <alice/megamind/library/sources/request.h>
#include <alice/megamind/library/speechkit/request.h>
#include <alice/megamind/library/util/status.h>

#include <alice/megamind/protos/proactivity/proactivity.pb.h>

#include <alice/library/json/json.h>
#include <alice/library/network/request_builder.h>

#include <dj/services/alisa_skills/server/proto/client/proactivity_response.pb.h>

#include <library/cpp/scheme/scheme.h>

#include <util/generic/maybe.h>
#include <util/generic/strbuf.h>

namespace NAlice::NMegamind {

constexpr inline TStringBuf PROACTIVITY_HISTORY = "/v1/personality/profile/alisa/kv/proactivity_history";

inline constexpr ui64 PROACTIVITY_TIME_DELTA_THRESHOLD = 4 * 60; // in minutes
inline constexpr ui64 PROACTIVITY_REQUEST_DELTA_THRESHOLD = 2;

TProactivityStorage GetProactivityStorage(const TSpeechKitRequest& skr, const ISession* session,
                                          const TProactivityStorage& mementoProactivityStorage, TRTLogger* logger = nullptr);

// Intent if there is one, otherwise scenario
TString GetProactivitySource(const ISession* session, const TScenarioResponse& response);

bool IsProactivityDisabledInApp(const TMaybe<NSc::TValue>& personalData);

TSourcePrepareStatus PrepareSkillProactivityRequest(const IContext& ctx,
                                                    const TRequest& requestModel,
                                                    const TScenarioToRequestFrames& scenarioToFrames,
                                                    const TProactivityStorage& storage,
                                                    NNetwork::IRequestBuilder& request);

TStatus ParseSkillProactivityResponse(const TString& response, NDJ::NAS::TProactivityResponse& outResponse);

TProactivityAnswer GetProactivityRecommendations(const NDJ::NAS::TProactivityResponse& response,
                                                 const TString& source, TRTLogger& logger);

template <typename TCtx>
bool IsProactivityAllowedInApp(const TCtx& ctx) {
    return ctx.ClientInfo().IsSmartSpeaker() || ctx.HasExpFlag(EXP_PROACTIVITY_SERVICE_ALL_APPS);
}

bool ProactivityHasFrequentSources(const TRequest& requestModel,
                                   const TScenarioToRequestFrames& scenarioToFrames);
} // NAlice::NMegamind

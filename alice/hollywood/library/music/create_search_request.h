#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>

#include <alice/library/json/json.h>
#include <alice/library/websearch/websearch.h>

#include <alice/hollywood/library/music/music_resources.h>

namespace NAlice::NHollywood::NMusic {

constexpr TStringBuf MUSIC_SEARCH_HTTP_REQUEST_ITEM = "music_search_http_request";

TString MergeTextFromSlots(const TMusicResources& musicResources, const TFrame& frame);

TEvent CreatePartialEvent(const TScenarioInputWrapper& input);

bool HaveHttpResponse(const TScenarioHandleContext& ctx, const TStringBuf itemName);

TString GetRawHttpResponse(const TScenarioHandleContext& ctx, const TStringBuf itemName);

TMaybe<TString> GetRawHttpResponseMaybe(TScenarioHandleContext& ctx, const TStringBuf itemName);

NAlice::TWebSearchBuilder ConstructSearchRequest(
    const NAlice::NHollywood::NMusic::TMusicResources& musicResources,
    const TFrame& frame,
    const TScenarioRunRequestWrapper& runRequest,
    const NScenarios::TInterfaces& interfaces,
    const NScenarios::TRequestMeta& requestMeta,
    TRTLogger& rtLogger,
    TString& encodedAliceMeta,
    bool waitAll
);

} // namespace NAlice::NHollywood

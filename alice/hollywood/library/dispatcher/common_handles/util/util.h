#pragma once

#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/metrics/metrics.h>

#include <alice/library/metrics/util.h>

#include <apphost/api/service/cpp/service.h>

namespace NAlice::NHollywood {

namespace NImpl {

enum class ERequestType {
    Run /* "run" */,
    Apply /* "apply" */,
    Commit /* "commit" */,
    Continue /* "continue" */,
};

} // namespace NImpl {

// Apphost item names
constexpr TStringBuf HTTP_RESPONSE_ITEM = "http_response";
constexpr TStringBuf REQUEST_META_ITEM = "mm_scenario_request_meta";


// http request/response json fields
constexpr TStringBuf CONTENT_KEY = "content";
constexpr TStringBuf HEADERS_KEY = "headers";
constexpr TStringBuf URI_KEY = "uri";


// Paths
const TString SOLOMON_PATH = "/counters/json";
const TString FAST_DATA_VERSION_PATH = "/fast_data_version";
const TString RELOAD_FAST_DATA_PATH = "/reload_fast_data";
const TString PING_PATH = "/ping";
const TString VERSION_PATH = "/version";
const TString VERSION_JSON_PATH = "/version_json";
const TString UTILITY_PATH = "/utility";
const TString REOPEN_LOGS_PATH = "/reopen_logs";

const TString SPLIT_WEB_SEARCH_PATH = "/split_web_search";


TString GetPath(const NJson::TJsonValue& httpRequest);

NJson::TJsonValue GetAppHostParams(const NAppHost::IServiceContext& ctx);

[[nodiscard]] NScenarios::TRequestMeta GetMeta(const NAppHost::IServiceContext& ctx, TRTLogger& logger);

TRTLogger CreateLogger(TGlobalContext& globalContext, const TString& token);

TString GetRTLogToken(const NJson::TJsonValue& appHostParams, const ui64 ruid);

void UpdateMiscHandleSensors(IGlobalContext& globalContext, const EMiscHandle handle,
                             const ERequestResult requestResult, const i64 timeMs);

} // namespace NAlice::NHollywood

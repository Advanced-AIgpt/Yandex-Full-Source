#pragma once

#include <alice/megamind/protos/scenarios/analytics_info.pb.h>

#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/json/json_value.h>
#include <library/cpp/scheme/scheme.h>

#include <util/generic/maybe.h>
#include <util/generic/string.h>

namespace NAlice {

namespace NScenarios {

TMaybe<TAnalyticsInfo> GetAnalyticsInfoFromBase64(const TStringBuf data);

TMaybe<TAnalyticsInfo> GetAnalyticsInfoFromBassResponse(const NJson::TJsonValue& state);
TMaybe<TAnalyticsInfo> GetAnalyticsInfoFromBassResponse(const NSc::TValue& state);

} // namespace NScenarios

namespace NAnalyticsInfo {

namespace NImpl {

inline const TCgiParameters COMMON_CGI_PARAMETERS = {
    {"init_meta", "use-src-new-tunneller"}, {"init_meta", "need_selected_with_address=1"},
    {"debug", "dump_sources_answer_stat"},  {"app_host_params", "need_debug_info=1"},
    {"init_meta", "need_debug_info=1"},     {"flag", "use-tunneller-soy-raw-response=1"}};

inline constexpr TStringBuf PROFILE_NAME_CGI_KEY = "flag";
inline constexpr TStringBuf PROFILE_NAME_CGI_VALUE_PREFIX = "restriction_profile=";

TString ConstructProfileNameByFlag(const TMaybe<TString>& tunnellerProfileFlag);

} // namespace NImpl

TCgiParameters ConstructWebSearchRequestCgiParameters(const TMaybe<TString>& tunnellerProfileFlag);
TCgiParameters ConstructVideoSearchRequestCgiParameters(const TMaybe<TString>& tunnellerProfileFlag);

} // namespace NAnalyticsInfo

} // namespace NAlice

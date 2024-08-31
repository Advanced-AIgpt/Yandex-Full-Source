#include "utils.h"

#include "names.h"

#include <alice/library/video_common/defs.h>

#include <library/cpp/string_utils/base64/base64.h>

#include <util/generic/maybe.h>
#include <util/string/builder.h>

namespace NAlice {

namespace NScenarios {

using namespace NAlice::NVideoCommon;

namespace {

constexpr TStringBuf BASS_BLOCKS_NAME = "blocks";
constexpr TStringBuf BASS_TYPE_NAME = "type";

constexpr TStringBuf BASS_TYPE_DATA_SCENARIO_ANALYTICS_INFO = "data";
constexpr TStringBuf BASS_TYPE_NAME_SCENARIO_ANALYTICS_INFO = "scenario_analytics_info";

} // namespace

TMaybe<TAnalyticsInfo> GetAnalyticsInfoFromBase64(const TStringBuf data) {
    if (!data) {
        return Nothing();
    }

    TAnalyticsInfo analyticsInfo;
    if (!analyticsInfo.ParseFromString(Base64DecodeUneven(data))) {
        return Nothing();
    }
    return MakeMaybe(std::move(analyticsInfo));
}

TMaybe<TAnalyticsInfo> GetAnalyticsInfoFromBassResponse(const NJson::TJsonValue& state) {
    TMaybe<TAnalyticsInfo> resultAnalyticsInfo;
    for (const auto& block : state[BASS_BLOCKS_NAME].GetArray()) {
        if (block.IsMap() && block[BASS_TYPE_NAME].GetString() == BASS_TYPE_NAME_SCENARIO_ANALYTICS_INFO) {
            resultAnalyticsInfo =
                GetAnalyticsInfoFromBase64(block[BASS_TYPE_DATA_SCENARIO_ANALYTICS_INFO].GetString());
            break;
        }
    }

    return resultAnalyticsInfo;
}

TMaybe<TAnalyticsInfo> GetAnalyticsInfoFromBassResponse(const NSc::TValue& state) {
    return GetAnalyticsInfoFromBassResponse(state.ToJsonValue());
}

} // namespace NScenarios

namespace NAnalyticsInfo {

namespace NImpl {

TString ConstructProfileNameByFlag(const TMaybe<TString>& tunnellerProfileFlag) {
    return TStringBuilder{} << PROFILE_NAME_CGI_VALUE_PREFIX
                            << (tunnellerProfileFlag ? *tunnellerProfileFlag : DEFAULT_TUNNELLER_PROFILE);
}

} // namespace NImpl

TCgiParameters ConstructWebSearchRequestCgiParameters(const TMaybe<TString>& tunnellerProfileFlag) {
    TCgiParameters parameters = NImpl::COMMON_CGI_PARAMETERS;
    parameters.InsertEscaped(NImpl::PROFILE_NAME_CGI_KEY, NImpl::ConstructProfileNameByFlag(tunnellerProfileFlag));
    return parameters;
}

TCgiParameters ConstructVideoSearchRequestCgiParameters(const TMaybe<TString>& tunnellerProfileFlag) {
    return ConstructWebSearchRequestCgiParameters(tunnellerProfileFlag.Defined() ?
                                                  tunnellerProfileFlag :
                                                  TMaybe<TString>(DEFAULT_VIDEO_TUNNELLER_PROFILE));
}

} // namespace NAnalyticsInfo

} // namespace NAlice

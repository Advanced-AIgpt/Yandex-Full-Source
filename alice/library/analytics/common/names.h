#pragma once

#include <util/generic/strbuf.h>

namespace NAlice::NAnalyticsInfo {

inline constexpr TStringBuf DEFAULT_TUNNELLER_PROFILE = "weak_consistency__web__desktop__production__tier0_tier1";
inline constexpr TStringBuf DEFAULT_VIDEO_TUNNELLER_PROFILE = "weak_consistency__video__desktop__hamster";
inline constexpr TStringBuf MEGAMIND_ANALYTICS_INFO = "megamind_analytics_info";
inline constexpr TStringBuf TUNNELLER_RAW_RESPONSE = "tunneller_raw_response";

} // namespace NAlice::NAnalyticsInfo

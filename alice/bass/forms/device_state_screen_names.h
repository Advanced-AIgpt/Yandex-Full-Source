#pragma once

#include <util/generic/strbuf.h>
#include <util/generic/string.h>

namespace NAlice {

// known client features
inline constexpr TStringBuf CLIENT_FEATURE_CURRENT_HDCP_LEVEL_NONE = "current_HDCP_level_none";
inline constexpr TStringBuf CLIENT_FEATURE_CURRENT_HDCP_LEVEL_1X = "current_HDCP_level_1X";
inline constexpr TStringBuf CLIENT_FEATURE_CURRENT_HDCP_LEVEL_2X = "current_HDCP_level_2X";
inline constexpr TStringBuf CLIENT_FEATURE_VIDEO_FORMAT_SD = "video_format_SD";
inline constexpr TStringBuf CLIENT_FEATURE_VIDEO_FORMAT_HD = "video_format_HD";
inline constexpr TStringBuf CLIENT_FEATURE_VIDEO_FORMAT_UHD = "video_format_UHD";
inline constexpr TStringBuf CLIENT_FEATURE_DYNAMIC_RANGE_SDR = "dynamic_range_SDR";
inline constexpr TStringBuf CLIENT_FEATURE_DYNAMIC_RANGE_HDR10 = "dynamic_range_HDR10";
inline constexpr TStringBuf CLIENT_FEATURE_DYNAMIC_RANGE_HDR10PLUS = "dynamic_range_HDR10Plus";
inline constexpr TStringBuf CLIENT_FEATURE_DYNAMIC_RANGE_DV = "dynamic_range_DV";
inline constexpr TStringBuf CLIENT_FEATURE_DYNAMIC_RANGE_HLG = "dynamic_range_HLG";

} // namespace NAlice

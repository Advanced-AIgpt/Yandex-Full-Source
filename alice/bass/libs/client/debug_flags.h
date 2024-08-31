#pragma once

#include <util/generic/strbuf.h>
#include <util/generic/string.h>

namespace NBASS {

// warning! will log personal channel request headers
inline constexpr TStringBuf DEBUG_FLAG_LOG_PERSONAL_TV_CHANNEL_REQUESTS = "DEBUG_log_personal_channel_requests";
// will add additional debug params to personal channel requests
inline constexpr TStringBuf DEBUG_FLAG_PERSONAL_TV_CHANNEL_REQUESTS = "DEBUG_personal_channel_requests";
// will include prestable channels into tv channels gallery
inline constexpr TStringBuf DEBUG_FLAG_TV_PRESTABLE_CHANNELS = "DEBUG_tv_prestable_channels";
// will exclude channels from gallery if provider name not matched
inline constexpr TStringBuf DEBUG_FLAG_VIDEO_PROVIDER_NAME_FILTER = "DEBUG_video_provider_name_filter";

} // namespace NBASS

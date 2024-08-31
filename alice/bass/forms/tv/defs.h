#pragma once

#include <alice/library/video_common/defs.h>

#include <util/generic/map.h>
namespace NBASS::NTvCommon {

using NAlice::NVideoCommon::DISABLE_PERSONAL_TV_CHANNEL;
using NAlice::NVideoCommon::QUASAR_FROM_ID;

constexpr TStringBuf QUASAR_SERVICE_ID = "ya-station";
constexpr TStringBuf QUASAR_PRESTABLE_SERVICE_ID = "ya-station-prestable";

constexpr TStringBuf QUASAR_PROXY_CHANNEL_TYPE = "quasar_proxy";

} // namespace NBASS::NTvCommon

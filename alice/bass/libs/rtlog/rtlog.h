#pragma once

#include <alice/bass/libs/config/config.sc.h>

#include <alice/rtlog/client/client.h>

#include <library/cpp/scheme/domscheme_traits.h>

namespace NBASS {
NRTLog::TClient ConstructRTLogClient(const NBASSConfig::TRTLogConfigConst<TSchemeTraits>& config);
}

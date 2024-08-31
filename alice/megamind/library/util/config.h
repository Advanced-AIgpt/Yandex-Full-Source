#pragma once

#include <alice/megamind/library/config/protos/config.pb.h>

#include <util/generic/yexception.h>
#include <util/system/types.h>

namespace NAlice {

ui16 MonitoringPort(const TConfig& config);

} // namespace NAlice

#pragma once

#include <alice/megamind/library/util/status.h>

#include <alice/protos/api/meta/backend.pb.h>

#include <library/cpp/logger/priority.h>


namespace NAlice::NMegamind {

TErrorOr<ELogPriority> MapUniproxyLogLevelToMegamindLogLevel(NAlice::ELogLevel uniproxyLogLevel);

} // namespace NAlice::NMegamind

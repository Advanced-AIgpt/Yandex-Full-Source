#pragma once

#include "alice/megamind/protos/common/frame.pb.h"
#include <alice/hollywood/library/frame/frame.h>

#include <alice/library/logger/logger.h>
#include <alice/megamind/protos/common/frame.pb.h>
#include <library/cpp/json/writer/json_value.h>

#include <util/generic/maybe.h>

namespace NAlice::NHollywood::NMusic {

TMaybe<TSemanticFrame> TryCreateOnDemandFairyTaleFrame(
    TRTLogger& logger,
    const TFrame* musicFairyTaleProto,
    const TFrame* musicPlayProto);

} // namespace NAlice::NHollywood::NMusic

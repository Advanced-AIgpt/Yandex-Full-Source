#pragma once

#include <alice/hollywood/library/music/fm_radio_resources.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/scenarios/music/common.h>
#include <alice/hollywood/library/scenarios/music/proto/music_arguments.pb.h>
#include <alice/hollywood/library/scenarios/music/util/util.h>

#include <alice/library/logger/logger.h>
#include <alice/library/music/defs.h>

namespace NAlice::NHollywood::NMusic {

std::unique_ptr<NScenarios::TScenarioRunResponse> HandleThinClientGenerative(
    TScenarioHandleContext& ctx,
    const TScenarioRunRequestWrapper& request,
    TNlgWrapper& nlg,
    TString generativeStationId,
    bool isNewContentRequestedByUser);

} // namespace NAlice::NHollywood::NMusic

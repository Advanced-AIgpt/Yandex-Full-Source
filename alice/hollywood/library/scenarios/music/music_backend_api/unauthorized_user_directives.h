#pragma once

#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/scenarios/music/proto/music_context.pb.h>

namespace NAlice::NHollywood::NMusic {

bool TryAddUnauthorizedUserDirectivesForThinClient(
    const NHollywood::TScenarioApplyRequestWrapper& applyRequest,
    const NMusic::TMusicContext musicContext,
    TResponseBodyBuilder& bodyBuilder
);

bool TryAddUnauthorizedUserDirectivesForThickClient(
    const NHollywood::TScenarioApplyRequestWrapper& applyRequest,
    const NJson::TJsonValue& bassStateJson,
    TResponseBodyBuilder& bodyBuilder
);

bool TryAddUnauthorizedUserDirectivesForVinsRunResponse(
    const NHollywood::TScenarioRunRequestWrapper& runRequest,
    const NJson::TJsonValue& bassStateJson,
    TResponseBodyBuilder& bodyBuilder
);

} // namespace NAlice::NHollywood::NMusic

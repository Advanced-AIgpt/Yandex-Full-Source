#pragma once

#include <alice/library/client/client_features.h>

#include <library/cpp/cgiparam/cgiparam.h>

namespace NAlice::NDirectGallery {

bool CanShowDirectGallery(const TClientFeatures& features);

bool CanShowDirectGallery(const TClientInfo& clientInfo, const THashMap<TString, TMaybe<TString>>& experiments);

TCgiParameters MakeDirectExperimentCgi(const TExpFlags& flags);

} // namespace NAlice::NDirectGallery

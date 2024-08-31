#pragma once

#include "playlist_id.h"

#include <alice/hollywood/library/scenarios/music/proto/music_context.pb.h>

namespace NAlice::NHollywood::NMusic {

TString SimilarRadioId(const TContentId& contentId);

} // namespace NAlice::NHollywood::NMusic

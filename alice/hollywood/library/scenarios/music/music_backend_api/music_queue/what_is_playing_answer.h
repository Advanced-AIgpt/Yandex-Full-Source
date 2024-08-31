#pragma once

#include <alice/hollywood/library/scenarios/music/proto/music_context.pb.h>

namespace NAlice::NHollywood::NMusic {

TString MakeWhatIsPlayingAnswer(const TQueueItem& item, const bool useTrack = false);

} // namespace NAlice::NHollywood::NMusic

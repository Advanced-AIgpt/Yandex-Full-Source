#pragma once

#include <alice/begemot/lib/trivial_tagger/proto/config.pb.h>
#include <alice/megamind/protos/common/frame.pb.h>

#include <util/generic/vector.h>

namespace NAlice {

TVector<TSemanticFrame> BuildTrivialTaggerFrames(const TTrivialTaggerConfig& config,
    const THashSet<TString>& experiments);

} // namespace NAlice

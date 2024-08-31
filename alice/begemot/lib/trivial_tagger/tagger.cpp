#include "tagger.h"

#include <alice/begemot/lib/utils/config.h>

#include <util/generic/hash_set.h>

namespace NAlice {

TVector<TSemanticFrame> BuildTrivialTaggerFrames(const TTrivialTaggerConfig& config,
    const THashSet<TString>& experiments)
{
    THashSet<TString> processedFrames;
    TVector<TSemanticFrame> frames;
    for (const TTrivialTaggerConfig::TFrame& frameConfig : config.GetFrames()) {
        if (!IsEnabledByExperiments(frameConfig.GetExperiments(), experiments)) {
            continue;
        }
        const TString& frameName = frameConfig.GetName();
        const auto [_, shouldProcess] = processedFrames.insert(frameName);
        if (!shouldProcess) {
            continue;
        }
        TSemanticFrame& frame = frames.emplace_back();
        frame.SetName(frameName);
        for (const TTrivialTaggerConfig::TSlot& slotConfig : frameConfig.GetSlots()) {
            TSemanticFrame::TSlot* slot = frame.AddSlots();
            slot->SetName(slotConfig.GetName());
            slot->SetType(slotConfig.GetType());
            slot->SetValue(slotConfig.GetValue());
        }
    }
    return frames;
}

} // namespace NAlice

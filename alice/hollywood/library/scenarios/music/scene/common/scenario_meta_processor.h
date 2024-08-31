#pragma once

#include <alice/hollywood/library/scenarios/music/proto/music.pb.h>
#include <alice/hollywood/library/scenarios/music/proto/music_context.pb.h>

#include "structs.h"

namespace NAlice::NHollywoodFw::NMusic {

class TScenarioMetaProcessor {
public:
    using TCurrentItemProcessor = std::function<void(TMusicScenarioRenderArgsCommon&,
                                                     const NHollywood::NMusic::TQueueItem&)>;

public:
    TScenarioMetaProcessor(TScenarioRequestData requestData);

    TScenarioMetaProcessor& SetCommandInfo(const TCommandInfo&);
    TScenarioMetaProcessor& SetCurrentItemProcessor(TCurrentItemProcessor);
    [[nodiscard]] TCommonRenderData Process();

private:
    TScenarioRequestData RequestData_;
    TScenarioStateData ScenarioStateData_;

    const TCommandInfo* CommandInfo_ = nullptr;
    TCurrentItemProcessor CurrentItemProcessor_ = {};
};

} // namespace NAlice::NHollywoodFw::NMusic

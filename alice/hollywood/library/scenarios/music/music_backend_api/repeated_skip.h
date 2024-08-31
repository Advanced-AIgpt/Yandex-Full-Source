#pragma once

#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/scenarios/music/proto/music_arguments.pb.h>
#include <alice/hollywood/library/scenarios/music/proto/music_context.pb.h>
#include <alice/library/logger/logger.h>

namespace NAlice::NHollywood::NMusic {

class TRepeatedSkip {
public:
    TRepeatedSkip(TScenarioState& state, TRTLogger& logger);

    void IncreaseCount();
    void ResetCount();

    void HandlePlayerCommand(const TMusicArguments::EPlayerCommand playerCommand);
    void HandlePlayerCommand(const TTypedSemanticFrame& tsf);

    bool MayPropose(const TScenarioBaseRequestWrapper& request) const;
    void SaidProposal(const TScenarioBaseRequestWrapper& request);
    bool TryPropose(const TScenarioBaseRequestWithInputWrapper& request, TResponseBodyBuilder& bodyBuilder, TNlgData& nlgData);

private:
    TScenarioState& State_;
    TRTLogger& Logger_;
    const bool InOnboarding_;
};

} // namespace NAlice::NHollywood::NMusic

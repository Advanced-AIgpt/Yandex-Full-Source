#pragma once

#include <alice/hollywood/library/scenarios/voiceprint/proto/voiceprint_arguments.pb.h>

#include <alice/hollywood/library/base_scenario/scenario.h>

#include <alice/megamind/protos/scenarios/request.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

namespace NAlice::NHollywood::NVoiceprint::NImpl {

class TApplyPrepareHandleImpl {
public:
    TApplyPrepareHandleImpl(TScenarioHandleContext& ctx);

    void Do();

private:
    const TVoiceprintArguments& GetApplyArgs() const;
    void LogInfoApplyArgs();

    bool HandleEnroll();
    bool HandleRemove();
    bool HandleSetMyName();

private:
    TScenarioHandleContext& Ctx_;
    TRTLogger& Logger_;
    NScenarios::TScenarioApplyRequest RequestProto_;
    const TScenarioApplyRequestWrapper Request_;
    TNlgWrapper Nlg_;
    TVoiceprintArguments ApplyArgs_;
};

} // namespace NAlice::NHollywood::NVoiceprint::NImpl

#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/megamind/protos/scenarios/request.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

#include <alice/hollywood/library/scenarios/voiceprint/proto/voiceprint.pb.h>
#include <alice/hollywood/library/scenarios/voiceprint/util/util.h>

#include <util/generic/hash.h>
#include <util/generic/string.h>

namespace NAlice::NHollywood::NVoiceprint::NImpl {

class TRunHandleImpl {
public:
    TRunHandleImpl(TScenarioHandleContext& ctx);

    void Do();

private:
    // impl.cpp
    void LogInfoScenarioState();
    bool CheckVoiceprintSupported();
    void IrrelevantResponse(EIrrelevantType type, TStringBuf msg = {});
    bool IsValidRegion();
    void RenderDeclineResponse();

    // enroll.cpp
    TMaybe<NScenarios::TScenarioRunResponse> HandleEnroll(bool isEnrollmentSuggested);

    // remove.cpp
    TMaybe<NScenarios::TScenarioRunResponse> HandleRemove();

    // set_my_name.cpp
    TMaybe<NScenarios::TScenarioRunResponse> HandleSetMyName();

    // what_is_my_name.cpp
    TMaybe<NScenarios::TScenarioRunResponse> HandleWhatIsMyName();
    NScenarios::TScenarioRunResponse HandleWhatIsMyNameUsingClientBiometry(const TFrame& frame,
                                                                           NJson::TJsonArray& renderSlots,
                                                                           TNlgData& nlgData);
    NScenarios::TScenarioRunResponse HandleWhatIsMyNameUsingServerBiometry(const TFrame& frame,
                                                                           NJson::TJsonArray& renderSlots,
                                                                           TNlgData& nlgData);
    NScenarios::TScenarioRunResponse RenderUnknownUser(const TFrame& frame,
                                                       NJson::TJsonArray& renderSlots,
                                                       TNlgData& nlgData,
                                                       bool shouldListen,
                                                       TStringBuf msg = {});
    NScenarios::TScenarioRunResponse RenderWhatIsMyNameResponse(const TFrame& frame,
                                                                const NJson::TJsonArray& renderSlots,
                                                                TNlgData& nlgData,
                                                                bool shouldListen);

private:
    TVoiceprintHandleContext VoiceprintCtx_;
};

} // namespace NAlice::NHollywood::NVoiceprint::NImpl

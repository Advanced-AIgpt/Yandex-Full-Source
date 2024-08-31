#pragma once
#include <alice/cuttlefish/library/experiments/local_experiments.h>
#include <alice/cuttlefish/library/experiments/event_patcher.h>
#include <alice/cuttlefish/library/protos/session.pb.h>
#include <library/cpp/json/json_value.h>
#include <util/stream/input.h>

namespace NVoice::NExperiments {

class TExperiments {
public:
    struct TConfig {
        TString ExperimentsFileName;
        TString MacrosFileName;
    };

public:
    TExperiments(const TConfig&);
    TExperiments(IInputStream& experimentsDataIn, IInputStream& macrosDataIn);
    TExperiments(const NJson::TJsonValue& experimentsJson, const NJson::TJsonValue& macrosJson);

    TEventPatcher CreatePatcherForSession(
        const NAliceProtocol::TSessionContext& sessionContext,
        const NJson::TJsonValue& initialEvent  //!< first Event in the session, expected to be System.SynchronizeState
    ) const;

private:
    TLocalExperiments ExperimentsStorage;
};

}  // namespace NVoice::NExperiments

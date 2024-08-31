#pragma once

#include <alice/megamind/library/apphost_request/protos/scenario.pb.h>
#include <alice/megamind/library/classifiers/pre.h>
#include <alice/megamind/library/scenarios/helpers/interface/scenario_ref.h>
#include <alice/megamind/library/scenarios/registry/interface/registry.h>
#include <alice/megamind/protos/common/frame.pb.h>

#include <util/generic/hash_set.h>
#include <util/generic/vector.h>

namespace NAlice {

class IRequestFrameToScenarioMatcher {
public:
    virtual ~IRequestFrameToScenarioMatcher() = default;

    virtual TScenarioToRequestFrames Match(const TVector<TSemanticFrame>& frames,
                                           const TScenarioRefs& scenarioRefs) const = 0;
};

THolder<IRequestFrameToScenarioMatcher>
CreateRequestFrameToScenarioMatcher(const THashSet<TString>& unrecognizableByWizardScenarios,
                                    const IScenarioRegistry::TFramesToScenarios& framesToScenarios);
THolder<IRequestFrameToScenarioMatcher>
CreateRequestFrameToScenarioMatcher(const IScenarioRegistry::TFramesToScenarios& framesToScenarios);

TScenarioToRequestFrames MatchScenarios(const THashMap<TString, NMegamindAppHost::TScenarioProto>& scenarios, const TScenarioRefs& scenarioRefs);

} // namespace NAlice

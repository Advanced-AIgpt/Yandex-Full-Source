#include "request_frame_to_scenario_matcher.h"

#include <alice/library/video_common/defs.h>
#include <alice/megamind/library/scenarios/defs/names.h>

#include <util/generic/algorithm.h>
#include <util/generic/hash_set.h>

#include <iterator>

namespace NAlice {

namespace {

const TStringBuf ACCEPTING_ALL_FRAMES_SCENARIOS[] = {
    MM_FACTS_SCENARIO,
    MM_GENERAL_CONVERSATION_SCENARIO
};

class TRequestFrameToScenarioMatcher final : public IRequestFrameToScenarioMatcher {
public:
    using TFramesToScenarios = IScenarioRegistry::TFramesToScenarios;

    TRequestFrameToScenarioMatcher(const THashSet<TString>& acceptingAllFramesScenarios,
                                   const TFramesToScenarios& framesToScenarios)
        : AcceptingAllFramesScenarios(acceptingAllFramesScenarios)
        , FramesToScenarios(framesToScenarios) {
    }

    TScenarioToRequestFrames Match(const TVector<TSemanticFrame>& frames,
                                   const TScenarioRefs& scenarioRefs) const override {
        TScenarioToRequestFrames result;
        THashMap<TString, size_t> acceptingOnlySpecifiedFramesScenarios;
        for (size_t i = 0; i < scenarioRefs.size(); ++i) {
            const auto& ref = scenarioRefs[i];
            const TString name = ref->GetScenario().GetName();
            if (AcceptingAllFramesScenarios.contains(name)) {
                result[ref] = frames;
            } else {
                result[ref] = {};
                acceptingOnlySpecifiedFramesScenarios[name] = i;
            }
        }

        for (const auto& frame : frames) {
            const auto* candidates = FramesToScenarios.FindPtr(frame.GetName());
            if (!candidates) {
                continue;
            }
            for (const auto& candidate : *candidates) {
                if (const auto* index = acceptingOnlySpecifiedFramesScenarios.FindPtr(candidate)) {
                    Y_ASSERT(*index < scenarioRefs.size());
                    result[scenarioRefs[*index]].push_back(frame);
                }
            }
        }

        return result;
    }

private:
    THashSet<TString> AcceptingAllFramesScenarios;
    const TFramesToScenarios& FramesToScenarios;
};

} // namespace

THolder<IRequestFrameToScenarioMatcher>
CreateRequestFrameToScenarioMatcher(const THashSet<TString>& acceptingAllFramesScenarios,
                                    const IScenarioRegistry::TFramesToScenarios& framesToScenarios) {
    return MakeHolder<TRequestFrameToScenarioMatcher>(acceptingAllFramesScenarios, framesToScenarios);
}

THolder<IRequestFrameToScenarioMatcher>
CreateRequestFrameToScenarioMatcher(const IScenarioRegistry::TFramesToScenarios& framesToScenarios) {
    static const THashSet<TString> acceptingAllFramesScenarios{std::begin(ACCEPTING_ALL_FRAMES_SCENARIOS),
                                                                   std::end(ACCEPTING_ALL_FRAMES_SCENARIOS)};
    return CreateRequestFrameToScenarioMatcher(acceptingAllFramesScenarios, framesToScenarios);
}

TScenarioToRequestFrames MatchScenarios(const THashMap<TString, NMegamindAppHost::TScenarioProto>& scenarios, const TScenarioRefs& scenarioRefs) {
    TScenarioToRequestFrames result;
    for (size_t i = 0; i < scenarioRefs.size(); ++i) {
        const auto& ref = scenarioRefs[i];
        const auto* scenario = scenarios.FindPtr(ref->GetScenario().GetName());
        if (!scenario) {
            continue;
        }

        auto& r = result[ref];
        for (const auto& frame : scenario->GetSemanticFrame()) {
            r.push_back(frame);
        }
    }

    return result;
}

} // namespace NAlice

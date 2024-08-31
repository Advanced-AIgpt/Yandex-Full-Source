#include "request_frame_to_scenario_matcher.h"

#include <alice/library/frame/builder.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/hash.h>
#include <util/generic/yexception.h>
#include <util/system/compiler.h>

namespace NAlice {

namespace {

class TFakeScenario : public TScenario {
public:
    TFakeScenario(TStringBuf name, const TVector<TString>& acceptedFrames)
        : TScenario(name)
        , AcceptedFrames(acceptedFrames)
    {
    }

    TVector<TString> GetAcceptedFrames() const override {
        return AcceptedFrames;
    }

private:
    TVector<TString> AcceptedFrames;
};

TVector<TSemanticFrame> MakeFrames(const TVector<TString>& names) {
    TVector<TSemanticFrame> result;
    for (const auto& name : names) {
        result.push_back(MakeFrame(name));
    }
    return result;
}

class TTestScenarioRef : public IScenarioRef {
public:
    TTestScenarioRef(TStringBuf name, const TVector<TString>& acceptedFrames)
        : ScenarioRef(MakeHolder<TFakeScenario>(name, acceptedFrames))
    {
        Y_ENSURE(ScenarioRef);
    }

    void Accept(const IScenarioVisitor& /* visitor */) const override {
        Y_UNREACHABLE();
    }

    const TScenario& GetScenario() const override {
        return *ScenarioRef;
    }

private:
    THolder<TScenario> ScenarioRef;
};

TVector<TIntrusivePtr<IScenarioRef>> MakeScenarioRefs(const THashMap<TString, TVector<TString>>& scenarios) {
    TVector<TIntrusivePtr<IScenarioRef>> result;
    for (auto&& [name, acceptedFrames] : scenarios) {
        result.push_back(MakeIntrusive<TTestScenarioRef>(name, acceptedFrames));
    }
    return result;
}

THashSet<TString> GetKeys(const THashMap<TString, TVector<TString>>& mapping) {
    THashSet<TString> keys;
    for (const auto& item : mapping) {
        keys.insert(item.first);
    }
    return keys;
}

THashMap<TString, TVector<TString>> Simplify(const TScenarioToRequestFrames& scenarioRefToFrames) {
    THashMap<TString, TVector<TString>> simplified;
    for (const auto& [scenarioRef, frames] : scenarioRefToFrames) {
        TVector<TString> frameNames;
        for (const auto& frame : frames) {
            frameNames.push_back(frame.GetName());
        }
        simplified[scenarioRef->GetScenario().GetName()] = frameNames;
    }

    return simplified;
}

} // namespace

Y_UNIT_TEST_SUITE(IRequestFramesToScenarioMatcher) {
    Y_UNIT_TEST(Match) {
        const THashMap<TString, TVector<TString>> scenarioToFrames{
            {"cancel", {"cancel"}},
            {"make_money", {"make_money"}},
            {"external", {"external"}},
            {"protocol", {"say_hello", "say_goodbye", "say_nothing"}}
        };
        const THashMap<TString, TVector<TString>> framesToScenarios = [&scenarioToFrames](){
            THashMap<TString, TVector<TString>> result;
            for (auto&& [scenario, frames] : scenarioToFrames) {
                for (const auto& frame : frames) {
                    result[frame].push_back(scenario);
                }
            }
            return result;
        }();

        const TVector<TString> frames{"cancel", "say_hello", "cancel", "say_goodbye", "other"};

        const auto matcher = CreateRequestFrameToScenarioMatcher({"external"}, framesToScenarios);

        const auto gotScenarioNameToFrameNames =
            Simplify(matcher->Match(MakeFrames(frames), MakeScenarioRefs(scenarioToFrames)));

        const THashMap<TString, TVector<TString>> expectedScenarioNameToFrameNames = {
            {"cancel", {"cancel", "cancel"}}, {"make_money", {}}, {"external", frames}, {"protocol", {"say_hello", "say_goodbye"}}};

        UNIT_ASSERT_VALUES_EQUAL(GetKeys(expectedScenarioNameToFrameNames), GetKeys(gotScenarioNameToFrameNames));
        for (const auto& [expectedScenarioName, expectedFrameNames] : expectedScenarioNameToFrameNames) {
            UNIT_ASSERT_VALUES_EQUAL_C(expectedFrameNames, gotScenarioNameToFrameNames.at(expectedScenarioName),
                                       "Scenario: " << expectedScenarioName);
        }
    }
}

} // namespace NAlice

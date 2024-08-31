#include "stack_engine.h"

#include <library/cpp/testing/unittest/registar.h>

namespace {

using namespace NAlice;
using namespace NAlice::NMegamind;

const TString FIRST_SCENARIO_NAME = "First";
const TString SECOND_SCENARIO_NAME = "Second";
const TString THIRD_SCENARIO_NAME = "Third";

void AddItem(TStackEngineCore& core, const TString& scenarioName) {
    auto& item = *core.AddItems();
    item.SetScenarioName(scenarioName);
}

IStackEngine::TItem NewItem(const TString& scenarioName) {
    IStackEngine::TItem item{};
    item.SetScenarioName(scenarioName);
    return item;
}

Y_UNIT_TEST_SUITE(StackEngine) {
    Y_UNIT_TEST(IsEmpty_EmptyCore) {
        TStackEngineCore core;
        TStackEngine stackEngine{core};

        UNIT_ASSERT(stackEngine.IsEmpty());
    }

    Y_UNIT_TEST(IsEmpty_CoreContainsItem) {
        TStackEngineCore core;
        core.AddItems();
        TStackEngine stackEngine{core};

        UNIT_ASSERT(!stackEngine.IsEmpty());
    }

    Y_UNIT_TEST(IsEmpty_EmptyCorePushItem) {
        TStackEngineCore core;
        TStackEngine stackEngine{core};
        stackEngine.Push(TStackEngine::TItem{});

        UNIT_ASSERT(!stackEngine.IsEmpty());
    }

    Y_UNIT_TEST(Peek) {
        TStackEngineCore core;
        AddItem(core, FIRST_SCENARIO_NAME);
        AddItem(core, SECOND_SCENARIO_NAME);
        TStackEngine stackEngine{core};

        UNIT_ASSERT_VALUES_EQUAL(stackEngine.Peek().GetScenarioName(), SECOND_SCENARIO_NAME);
        UNIT_ASSERT(!stackEngine.IsUpdated());
    }

    Y_UNIT_TEST(Pop) {
        TStackEngineCore core;
        AddItem(core, FIRST_SCENARIO_NAME);
        AddItem(core, SECOND_SCENARIO_NAME);
        TStackEngine stackEngine{core};

        UNIT_ASSERT_VALUES_EQUAL(stackEngine.Pop().GetScenarioName(), SECOND_SCENARIO_NAME);
        UNIT_ASSERT_VALUES_EQUAL(stackEngine.Peek().GetScenarioName(), FIRST_SCENARIO_NAME);
        UNIT_ASSERT_VALUES_EQUAL(stackEngine.Pop().GetScenarioName(), FIRST_SCENARIO_NAME);
        UNIT_ASSERT(stackEngine.IsEmpty());
        UNIT_ASSERT(stackEngine.IsUpdated());
    }

    Y_UNIT_TEST(PopScenario) {
        TStackEngineCore core;
        AddItem(core, FIRST_SCENARIO_NAME);
        AddItem(core, FIRST_SCENARIO_NAME);
        AddItem(core, SECOND_SCENARIO_NAME);
        AddItem(core, SECOND_SCENARIO_NAME);
        AddItem(core, SECOND_SCENARIO_NAME);
        AddItem(core, THIRD_SCENARIO_NAME);
        AddItem(core, THIRD_SCENARIO_NAME);
        AddItem(core, THIRD_SCENARIO_NAME);
        TStackEngine stackEngine{core};

        stackEngine.PopScenario(FIRST_SCENARIO_NAME);
        stackEngine.PopScenario(SECOND_SCENARIO_NAME);
        UNIT_ASSERT_VALUES_EQUAL(stackEngine.Peek().GetScenarioName(), THIRD_SCENARIO_NAME);
        UNIT_ASSERT(!stackEngine.IsUpdated());

        stackEngine.PopScenario(THIRD_SCENARIO_NAME);
        UNIT_ASSERT_VALUES_EQUAL(stackEngine.Peek().GetScenarioName(), SECOND_SCENARIO_NAME);
        UNIT_ASSERT(stackEngine.IsUpdated());

        UNIT_ASSERT_VALUES_EQUAL(stackEngine.Pop().GetScenarioName(), SECOND_SCENARIO_NAME);
        stackEngine.PopScenario(SECOND_SCENARIO_NAME);
        UNIT_ASSERT_VALUES_EQUAL(stackEngine.Peek().GetScenarioName(), FIRST_SCENARIO_NAME);
        UNIT_ASSERT(stackEngine.IsUpdated());

        stackEngine.PopScenario(FIRST_SCENARIO_NAME);
        UNIT_ASSERT(stackEngine.IsEmpty());
        UNIT_ASSERT(stackEngine.IsUpdated());
    }

    Y_UNIT_TEST(Push) {
        TStackEngine stackEngine{};

        stackEngine.Push(NewItem(FIRST_SCENARIO_NAME));
        stackEngine.Push(NewItem(THIRD_SCENARIO_NAME));
        stackEngine.Push(NewItem(SECOND_SCENARIO_NAME));

        UNIT_ASSERT_VALUES_EQUAL(stackEngine.Pop().GetScenarioName(), SECOND_SCENARIO_NAME);
        UNIT_ASSERT_VALUES_EQUAL(stackEngine.Pop().GetScenarioName(), THIRD_SCENARIO_NAME);
        UNIT_ASSERT_VALUES_EQUAL(stackEngine.Pop().GetScenarioName(), FIRST_SCENARIO_NAME);
        UNIT_ASSERT(stackEngine.IsEmpty());
        UNIT_ASSERT(stackEngine.IsUpdated());
    }

    Y_UNIT_TEST(ReleaseCore) {
        TStackEngine stackEngine{};

        stackEngine.Push(NewItem(FIRST_SCENARIO_NAME));
        stackEngine.Push(NewItem(SECOND_SCENARIO_NAME));

        const auto core = std::move(stackEngine).ReleaseCore();
        const auto items = core.GetItems();

        UNIT_ASSERT_VALUES_EQUAL(items.size(), 2);
        UNIT_ASSERT_VALUES_EQUAL(items[0].GetScenarioName(), FIRST_SCENARIO_NAME);
        UNIT_ASSERT_VALUES_EQUAL(items[1].GetScenarioName(), SECOND_SCENARIO_NAME);
    }
}

} // namespace

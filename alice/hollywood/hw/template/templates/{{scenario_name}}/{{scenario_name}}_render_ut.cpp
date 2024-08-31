#include "{{scenario_name}}_scene.h"

#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/hollywood/library/scenarios/{{scenario_name}}/proto/{{scenario_name}}.pb.h>

#include <alice/hollywood/library/framework/framework_ut.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywoodFw::N{{ScenarioName}} {

namespace {


} // anonimous namespace

Y_UNIT_TEST_SUITE({{ScenarioName}}Render) {

    Y_UNIT_TEST({{ScenarioName}}Render) {
        TTestEnvironment testData(NProductScenarios::{{SCENARIO_NAME}}, "ru-ru");
        T{{ScenarioName}}RenderArgs args;

        UNIT_ASSERT(testData >> TTestRender(&T{{ScenarioName}}Scene::Render, args) >> testData);
        UNIT_ASSERT(!testData.IsIrrelevant());
        UNIT_ASSERT_STRING_CONTAINS(testData.GetText(), "Hello, World!");
        UNIT_ASSERT_STRING_CONTAINS(testData.GetVoice(), "Hello, World!");
    }
}

} // namespace NAlice::NHollywoodFw::N{{ScenarioName}}

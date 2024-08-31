#include "test_scenario_scene.h"

#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/hollywood/library/scenarios/test_scenario/proto/test_scenario.pb.h>

#include <alice/hollywood/library/framework/framework_ut.h>
#include <alice/hollywood/library/util/service_context.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywoodFw::NTestScenario {

namespace {


} // anonimous namespace

Y_UNIT_TEST_SUITE(TestScenarioRender) {

    Y_UNIT_TEST(TestScenarioRender) {
        TTestEnvironment testData(NProductScenarios::TEST_SCENARIO, "ru-ru");
        testData.RunRequest.MutableInput()->MutableText()->SetUtterance("Repeat");
        testData.AddAnswer(NHollywood::REQUEST_ITEM, testData.RunRequest);
        testData.AddExp("test_scenario_logging=meta", "1");

        UNIT_ASSERT(testData >> TTestApphost("run") >> testData);
        Cerr << "ERROR: " << testData.GetErrorInfo().GetDetails() << Endl;
        UNIT_ASSERT(testData.IsIrrelevant());
        UNIT_ASSERT(testData.ContainsText("Repeat"));
    }
}

} // namespace NAlice::NHollywoodFw::NTestScenario

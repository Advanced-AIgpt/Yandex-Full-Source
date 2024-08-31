#include "blueprints_scene.h"

#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/hollywood/library/scenarios/blueprints/proto/blueprints.pb.h>

#include <alice/hollywood/library/framework/framework_ut.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywoodFw::NBlueprints {

namespace {


} // anonimous namespace

Y_UNIT_TEST_SUITE(BlueprintsRender) {

    Y_UNIT_TEST(BlueprintsRender) {
        TTestEnvironment testData(NProductScenarios::BLUEPRINTS, "ru-ru");
        TBlueprintsArgs args;
        UNIT_ASSERT(testData >> TTestRender(&TBlueprintsScene::Render, args) >> testData);
        UNIT_ASSERT(!testData.IsIrrelevant());
    }
}

} // namespace NAlice::NHollywoodFw::NBlueprints

#include "blueprints.h"
#include "blueprints_scene.h"

#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/hollywood/library/scenarios/blueprints/proto/blueprints.pb.h>

#include <alice/hollywood/library/framework/framework_ut.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywoodFw::NBlueprints {

Y_UNIT_TEST_SUITE(BlueprintsDispatch) {

    Y_UNIT_TEST(BlueprintsDispatch) {
        TTestEnvironment testData(NProductScenarios::BLUEPRINTS, "ru-ru");
        // Still not yet implemented
        UNIT_ASSERT(!(testData >> TTestDispatch(&TBlueprintsScenario::Dispatch) >> testData));
    }
}

} // namespace NAlice::NHollywoodFw::NBlueprints

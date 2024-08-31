#include "order_scene.h"

#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/hollywood/library/scenarios/order/proto/order.pb.h>

#include <alice/hollywood/library/framework/framework_ut.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywoodFw::NOrder {

namespace {


} // anonimous namespace

Y_UNIT_TEST_SUITE(OrderRender) {

    Y_UNIT_TEST(OrderRender) {
        TTestEnvironment testData(NProductScenarios::ORDER, "ru-ru");
        TOrderRenderArgs args;

        UNIT_ASSERT(testData >> TTestRender(&TOrderScene::Render, args) >> testData);
        UNIT_ASSERT(!testData.IsIrrelevant());
    }
}

} // namespace NAlice::NHollywoodFw::NOrder
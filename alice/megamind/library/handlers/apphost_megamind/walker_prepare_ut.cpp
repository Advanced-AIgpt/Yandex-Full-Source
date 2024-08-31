#include "walker_prepare.h"

#include "ut_helper.h"

#include <alice/megamind/library/apphost_request/item_names.h>
#include <alice/megamind/library/apphost_request/protos/scenario.pb.h>
#include <alice/megamind/library/testing/speechkit.h>
#include <alice/megamind/library/testing/speechkit_api_builder.h>

#include <library/cpp/testing/unittest/registar.h>

namespace {

using namespace NAlice;
using namespace NAlice::NMegamind;
using namespace NAlice::NMegamind::NTesting;

Y_UNIT_TEST_SUITE(AppHostMegamindWalkerPrepare) {
    Y_UNIT_TEST_F(Smoke, TAppHostWalkerTestFixture) {
        static const TString scenarioName = "test_scenario_1";

        RegisterScenario(scenarioName);
        InitBegemot("/begemot_native_proto_response");

        TSpeechKitApiRequestBuilder apiBuilder;
        apiBuilder.SetTextInput("привет");
        InitSkr(TSpeechKitRequestBuilder{apiBuilder.BuildJson()});

        {
            TWalkerPtr walker = MakeIntrusive<TCommonScenarioWalker>(GlobalCtx);
            TAppHostWalkerPrepareNodeHandler handler{GlobalCtx, walker};
            handler.RunSync(AhCtx.TestCtx());
        }

        const auto& items = AhCtx.TestCtx().GetProtobufItemRefs(AH_ITEM_SCENARIO);
        UNIT_ASSERT(items.size() == 1);
        NMegamindAppHost::TScenarioProto proto;
        UNIT_ASSERT(items.front().Fill(&proto));
        UNIT_ASSERT_VALUES_EQUAL(proto.GetName(), scenarioName);
    }
}

} // namespace

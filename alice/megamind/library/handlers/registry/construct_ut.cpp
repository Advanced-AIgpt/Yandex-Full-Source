#include "construct.h"

#include <alice/megamind/library/testing/fake_registry.h>
#include <alice/megamind/library/testing/mock_global_context.h>
#include <alice/megamind/library/util/wildcards.h>

#include <library/cpp/testing/gmock_in_unittest/gmock.h>
#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NMegamind {

using namespace testing;

const TString EXPECTED_APP_HOST_HANDLERS[] = {
    "/mm_combinators_setup",
    "/mm_fallback_response",
    "/mm_utterance_post_setup",
    "/mm_walker_apply_prepare_scenario",
    "/mm_walker_apply_render_scenario",
    "/mm_walker_monitoring",
    "/mm_walker_preclassify",
    "/mm_walker_prepare",
    "/ping",
    "/reload_logs",
    "/speechkit_request",
    "/utility",
    "/version",
    "/version_json",
};



Y_UNIT_TEST_SUITE(TestConstruct) {
    Y_UNIT_TEST(TestPopulateRegistry) {
        TConfig config{};
        TMockGlobalContext globalCtx{};
        TFakeRegistry registry;
        TScenarioConfigRegistry scenarioConfigRegistry{};
        TMaybe<NInfra::TLogger> udpLogger;
        TMaybe<NUdpClickMetrics::TSelfBalancingClient> udpClient;

        EXPECT_CALL(globalCtx, Config()).WillRepeatedly(ReturnRef(config));
        EXPECT_CALL(globalCtx, ScenarioConfigRegistry()).WillRepeatedly(ReturnRef(scenarioConfigRegistry));

        PopulateRegistry(globalCtx, udpLogger, udpClient, registry);
        // TODO(@petrk): Add reasonable checks (i.e. foreach handler : handlers { handle({}) })

        for (const auto& appHostHandlerPath : EXPECTED_APP_HOST_HANDLERS) {
            UNIT_ASSERT_C(registry.AppHostHandlers.contains(appHostHandlerPath),
                          "Unable to find app host handler: " << appHostHandlerPath);
        }
    }
}

} // namespace NAlice::NMegamind

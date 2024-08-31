#include "config_registry.h"

#include <alice/megamind/library/config/scenario_protos/common.pb.h>

#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NMegamind {

namespace {

inline const auto CONFIGS_PATH =
    TFsPath(ArcadiaSourceRoot()) / "alice/megamind/library/scenarios/config_registry/ut/combinators";

} // namespace

Y_UNIT_TEST_SUITE(TestCombinatorConfigRegistry) {
    Y_UNIT_TEST(CombinatorCheckStructure) {
        TCombinatorConfigRegistry registry{};
        for (const auto& [_, config] : LoadCombinatorConfigs(CONFIGS_PATH, /* validateConfigs= */ true)) {
            registry.AddCombinatorConfig(TCombinatorConfig{config});
        }

        UNIT_ASSERT_VALUES_EQUAL(registry.GetCombinatorConfigs().size(), 1);

        const auto& it = registry.GetCombinatorConfigs().find("TestConfig");
        UNIT_ASSERT(it != registry.GetCombinatorConfigs().end());
        const auto& instance = it->second;

        auto semanticFrames = instance.GetAcceptedFrames();
        UNIT_ASSERT_VALUES_EQUAL(semanticFrames.size(), 1);
        UNIT_ASSERT_VALUES_EQUAL(semanticFrames[0], "scenarios.sfs.test");

        auto scenarios = instance.GetAcceptedScenarios();
        UNIT_ASSERT_VALUES_EQUAL(scenarios.size(), 1);
        UNIT_ASSERT_VALUES_EQUAL(scenarios[0], "Scenario");

        UNIT_ASSERT(instance.GetEnabled());

        auto responibles = instance.GetResponsibles();
        UNIT_ASSERT_VALUES_EQUAL(responibles.GetLogins().size(), 1);
        UNIT_ASSERT_VALUES_EQUAL(responibles.GetLogins()[0], "Login");
        UNIT_ASSERT_VALUES_EQUAL(responibles.GetAbcServices().size(), 1);
        UNIT_ASSERT_VALUES_EQUAL(responibles.GetAbcServices()[0].GetName(), "Abc");
    }

    Y_UNIT_TEST(CombinatorEnsureUnique) {
        TCombinatorConfigProto aProto;
        aProto.SetName("foo");
        TCombinatorConfig a{aProto};
        TCombinatorConfigRegistry registry{};
        registry.AddCombinatorConfig(a);
        UNIT_ASSERT_EXCEPTION(registry.AddCombinatorConfig(a), yexception);
    }
}

} // namespace NAlice::NMegamind

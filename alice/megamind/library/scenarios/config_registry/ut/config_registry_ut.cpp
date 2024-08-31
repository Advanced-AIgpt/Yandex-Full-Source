#include "config_registry.h"

#include <alice/megamind/library/config/scenario_protos/common.pb.h>
#include <alice/megamind/library/config/scenario_protos/config.pb.h>
#include <alice/megamind/protos/common/data_source_type.pb.h>
#include <alice/protos/data/language/language.pb.h>

#include <library/cpp/langs/langs.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NMegamind {

namespace {

inline const auto CONFIGS_PATH =
    TFsPath(ArcadiaSourceRoot()) / "alice/megamind/library/scenarios/config_registry/ut/scenarios";

} // namespace

Y_UNIT_TEST_SUITE(TestScenarioConfigRegistry) {
    Y_UNIT_TEST(CheckStructure) {
        TScenarioConfigRegistry registry{};
        for (const auto& [_, config] : LoadScenarioConfigs(CONFIGS_PATH, /* validateConfigs= */ true)) {
            registry.AddScenarioConfig(config);
        }

        UNIT_ASSERT_VALUES_EQUAL(registry.GetScenarioConfigs().size(), 1);

        const auto& it = registry.GetScenarioConfigs().find("TestConfig");
        UNIT_ASSERT(it != registry.GetScenarioConfigs().end());
        const auto& instance = it->second;

        auto langs = instance.GetLanguages();
        UNIT_ASSERT_VALUES_EQUAL(langs.size(), 1);
        UNIT_ASSERT_EQUAL_C(langs[0], ELang::L_RUS, "ELang::L_RUS expected, but found: " << langs[0]);

        auto sources = instance.GetDataSources();
        TVector<TDataSourceParams> expectedSources;
        for (EDataSourceType type : {EDataSourceType::VINS_WIZARD_RULES, EDataSourceType::WEB_SEARCH_DOCS}) {
            TDataSourceParams params;
            params.SetType(type);
            params.SetIsRequired(type == VINS_WIZARD_RULES);
            expectedSources.push_back(std::move(params));
        }

        UNIT_ASSERT_VALUES_EQUAL(sources.size(), expectedSources.size());
        for (size_t i = 0; i < expectedSources.size(); ++i) {
            UNIT_ASSERT_EQUAL(sources[i].GetType(), expectedSources[i].GetType());
        }

        auto semanticFrames = instance.GetAcceptedFrames();
        UNIT_ASSERT_VALUES_EQUAL(semanticFrames.size(), 1);
        UNIT_ASSERT_VALUES_EQUAL(semanticFrames[0], "scenarios.sfs.test");

        auto handlers = instance.GetHandlers();
        UNIT_ASSERT_VALUES_EQUAL(handlers.GetBaseUrl(), "http://example.com");

        UNIT_ASSERT(instance.GetEnabled());

        auto responibles = instance.GetResponsibles();
        UNIT_ASSERT_VALUES_EQUAL(responibles.GetLogins().size(), 1);
        UNIT_ASSERT_VALUES_EQUAL(responibles.GetLogins()[0], "Login");
        UNIT_ASSERT_VALUES_EQUAL(responibles.GetAbcServices().size(), 1);
        UNIT_ASSERT_VALUES_EQUAL(responibles.GetAbcServices()[0].GetName(), "Abc");
    }

    Y_UNIT_TEST(EnsureUnique) {
        TScenarioConfig a;
        a.SetName("foo");
        TScenarioConfigRegistry registry{};
        registry.AddScenarioConfig(a);
        UNIT_ASSERT_EXCEPTION(registry.AddScenarioConfig(a), yexception);
    }
}

} // namespace NAlice::NMegamind

#include "config_ut_helpers.h"

#include <alice/megamind/library/config/config.h>
#include <alice/megamind/library/config/scenario_protos/combinator_config.pb.h>
#include <alice/megamind/library/config/scenario_protos/common.pb.h>
#include <alice/megamind/library/scenarios/config_registry/config_registry.h>
#include <alice/megamind/library/scenarios/config_registry/config_validator.h>
#include <alice/megamind/library/scenarios/helpers/scenario_api_helper.h>

#include <alice/library/network/common.h>

#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/folder/iterator.h>
#include <util/folder/path.h>
#include <util/generic/hash.h>
#include <util/generic/string.h>

#include <algorithm>

namespace NAlice::NMegamind {

using namespace NTesting;

class TCombinatorFixture : public NUnitTest::TBaseFixture {
public:
    TCombinatorFixture()
        : Production(LoadCombinatorConfigs(GetCombinatorConfigPath(ENV_PRODUCTION), /* validateConfigs= */ true))
        , Hamster(LoadCombinatorConfigs(GetCombinatorConfigPath(ENV_HAMSTER), /* validateConfigs= */ true))
        , Rc(LoadCombinatorConfigs(GetCombinatorConfigPath(ENV_RC), /* validateConfigs= */ true))
        , Dev(LoadCombinatorConfigs(GetCombinatorConfigPath(ENV_DEV), /* validateConfigs= */ true)) {
    }

    [[nodiscard]] const TCombinatorConfigs& GetProductionConfigs() const {
        return Production;
    }

    [[nodiscard]] const TCombinatorConfigs& GetHamsterConfigs() const {
        return Hamster;
    }

    [[nodiscard]] const TCombinatorConfigs& GetRcConfigs() const {
        return Rc;
    }

    [[nodiscard]] const TCombinatorConfigs& GetDevConfigs() const {
        return Dev;
    }

    [[nodiscard]] THashMap<TString, TCombinatorConfigs> GetConfigs() const {
        return {{ENV_PRODUCTION, GetProductionConfigs()},
                {ENV_HAMSTER, GetHamsterConfigs()},
                {ENV_RC, GetRcConfigs()},
                {ENV_DEV, GetDevConfigs()}};
    }

private:
    TCombinatorConfigs Production;
    TCombinatorConfigs Hamster;
    TCombinatorConfigs Rc;
    TCombinatorConfigs Dev;
};

Y_UNIT_TEST_SUITE_F(ValidateCombinatorConfigs, TCombinatorFixture) {
    Y_UNIT_TEST(ProductionContainsCentaurCombinatorConfig) {
        bool found = false;
        for (const auto& [_, config] : GetProductionConfigs()) {
            found = found || config.GetName() == "CentaurCombinator";
        }
        UNIT_ASSERT_C(found, "Unable to find CentaurCombinator in combinators");
    }

    Y_UNIT_TEST(EnsureHamsterAndProductionContainsTheSameSetOfCombinators) {
        EnsureSameSetOfConfigs(GetProductionConfigs(), GetHamsterConfigs(), ENV_PRODUCTION, ENV_HAMSTER);
        EnsureSameSetOfConfigs(GetHamsterConfigs(), GetProductionConfigs(), ENV_HAMSTER, ENV_PRODUCTION);
    }

    Y_UNIT_TEST(EnsureRcAndProductionContainsTheSameSetOfCombinators) {
        EnsureSameSetOfConfigs(GetProductionConfigs(), GetRcConfigs(), ENV_PRODUCTION, ENV_RC);
        EnsureSameSetOfConfigs(GetRcConfigs(), GetProductionConfigs(), ENV_RC, ENV_PRODUCTION);
    }

    Y_UNIT_TEST(CheckProductionCombinatorsProps) {
        TStringBuilder error{};
        for (const auto& [env, configs] : GetConfigs()) {
            if (env == ENV_DEV) {
                continue;
            }
            for (const auto& [file, config] : configs) {
                CHECK_FAILURE(config.GetResponsibles().GetAbcServices().size() == 0,
                    "No abc service provided, please add 'AbcServices' field to Responsibles");
                for (const auto& frame : config.GetAcceptedFrames()) {
                    CHECK_FAILURE(!(frame.StartsWith("personal_assistant.") ||
                                    frame.StartsWith("alice.") ||
                                    frame.StartsWith("quasar.")),
                                  "Frames must be named like alice.* or personal_assistant.*, but " << frame << " found");
                }
                CHECK_FAILURE(config.GetAcceptedFrames().empty() && !config.GetAcceptsAllFrames(),
                              "Unable to found entry for this combinator, one of AcceptedFrames and AcceptsAnyFrame "
                                  << "fields must be set");
                CHECK_FAILURE(config.GetAcceptedScenarios().empty() && !config.GetAcceptsAllScenarios(),
                              "Unable to found entry for this combinator, one of AcceptedScenarios and AcceptsAllScenarios "
                                  << "fields must be set");
            }
        }
        UNIT_ASSERT_C(error.empty(), error);
    }

    Y_UNIT_TEST(TestCombinatorsCheckFailure) {
        TStringBuilder error{};
        const auto file = "File";
        const auto config = []() {
            TCombinatorConfigProto configProto;
            auto* responsibles = configProto.MutableResponsibles();
            *responsibles->AddLogins() = "Owner";
            responsibles->AddAbcServices()->SetName("service");
            return TCombinatorConfig{configProto};
        }();
        const auto env = "Env";
        CHECK_FAILURE(true, "Hint");
        UNIT_ASSERT_VALUES_EQUAL(ToString(error), "[Env] File: Hint | Owners: @Owner, @abc:service\n");
    }

    Y_UNIT_TEST(TestCombinatorConfigsHaveSameEnableStatus) {
        TestConfigsHaveSameEnableStatus(GetConfigs());
    }

    Y_UNIT_TEST(TestOnlyDevCombinatorConfigsShouldBeDisabled) {
        TestOnlyDevConfigsShouldBeDisabled(GetProductionConfigs(), GetDevConfigs());
    }

    Y_UNIT_TEST(TestDescriptionIsProvidedInProductionCombinatorConfigs) {
        TStringBuilder error{};
        const auto env = ENV_PRODUCTION;
        for (const auto& [file, config] : GetProductionConfigs()) {
            CHECK_FAILURE(config.GetDescription().empty(), "Description should be provided in production combinators");
        }
        if (!error.empty()) {
            error << "Example: "
                  << R"(Description: "Комбинатор, который может объединить ответ нескольких сценариев")";
        }
        UNIT_ASSERT_C(error.empty(), error);
    }

    Y_UNIT_TEST(TestCombinatorAndFileNames) {
        TestConfigAndFileNames(GetConfigs());
    }

    Y_UNIT_TEST(CheckCombinatorCallbacks) {
        for (const auto& [env, configs] : GetConfigs()) {
            const TScenarioConfigs scenarioConfigs =
                LoadScenarioConfigs(GetScenarioConfigPath(env), /* validateConfigs= */ true);
            THashSet<TString> scenarioNames;
            for (const auto& [_, config] : scenarioConfigs) {
                scenarioNames.insert(config.GetName());
            }
            for (const auto& [file, config] : configs) {
                 for (const auto& callback : config.GetAcceptedTypedCallbacks()) {
                     UNIT_ASSERT_C(IsExistingTypedCallback(callback.GetCallbackName()),
                                   "There is no typed callback with name = \"" + callback.GetCallbackName() + "\"");
                     for (const auto& callbackScenario : callback.GetScenarios()) {
                        UNIT_ASSERT_C(scenarioNames.contains(callbackScenario),
                                      "There is no scenario " + callbackScenario + " used in " + callback.GetCallbackName() + " callback");
                     }
                 }
            }
        }
    }
}

} // namespace NAlice::NMegamind

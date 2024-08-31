#include "config_ut_helpers.h"

#include <alice/megamind/library/classifiers/formulas/protos/formulas_description.pb.h>
#include <alice/megamind/library/config/config.h>
#include <alice/megamind/library/config/scenario_protos/common.pb.h>
#include <alice/megamind/library/config/scenario_protos/config.pb.h>
#include <alice/megamind/library/scenarios/config_registry/config_registry.h>
#include <alice/megamind/library/scenarios/config_registry/config_validator.h>
#include <alice/megamind/library/scenarios/defs/names.h>
#include <alice/megamind/library/scenarios/helpers/scenario_api_helper.h>
#include <alice/megamind/library/testing/utils.h>
#include <alice/megamind/protos/quality_storage/storage.pb.h>

#include <alice/library/network/common.h>
#include <alice/protos/data/language/language.pb.h>

#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/folder/iterator.h>
#include <util/folder/path.h>
#include <util/generic/hash.h>
#include <util/generic/string.h>

#include <algorithm>

namespace NAlice::NMegamind {

using namespace NTesting;

class TFixture : public NUnitTest::TBaseFixture {
public:
    TFixture()
        : Production(LoadScenarioConfigs(GetScenarioConfigPath(ENV_PRODUCTION), /* validateConfigs= */ true))
        , Hamster(LoadScenarioConfigs(GetScenarioConfigPath(ENV_HAMSTER), /* validateConfigs= */ true))
        , Rc(LoadScenarioConfigs(GetScenarioConfigPath(ENV_RC), /* validateConfigs= */ true))
        , Dev(LoadScenarioConfigs(GetScenarioConfigPath(ENV_DEV), /* validateConfigs= */ true))
        , ClassificationConfig(GetRealClassificationConfig())
    {
    }

    [[nodiscard]] const TScenarioConfigs& GetProductionConfigs() const {
        return Production;
    }

    [[nodiscard]] const TScenarioConfigs& GetHamsterConfigs() const {
        return Hamster;
    }

    [[nodiscard]] const TScenarioConfigs& GetRcConfigs() const {
        return Rc;
    }

    [[nodiscard]] const TScenarioConfigs& GetDevConfigs() const {
        return Dev;
    }

    [[nodiscard]] THashMap<TString, TScenarioConfigs> GetConfigs() const {
        return {{ENV_PRODUCTION, GetProductionConfigs()},
                {ENV_HAMSTER, GetHamsterConfigs()},
                {ENV_RC, GetRcConfigs()},
                {ENV_DEV, GetDevConfigs()}};
    }

    [[nodiscard]] const TClassificationConfig& GetClassificationConfig() const {
        return ClassificationConfig;
    }

private:
    TScenarioConfigs Production;
    TScenarioConfigs Hamster;
    TScenarioConfigs Rc;
    TScenarioConfigs Dev;
    TClassificationConfig ClassificationConfig;
};

Y_UNIT_TEST_SUITE_F(ValidateConfigs, TFixture) {
    Y_UNIT_TEST(ProductionContainsDialogovoConfig) {
        bool found = false;
        for (const auto& [_, config] : GetProductionConfigs()) {
            found = found || config.GetName() == MM_DIALOGOVO_SCENARIO;
        }
        UNIT_ASSERT_C(found, "Unable to find Dialogovo in scenarios");
    }

    Y_UNIT_TEST(EnsureHamsterAndProductionContainsTheSameSetOfScenarios) {
        EnsureSameSetOfConfigs(GetProductionConfigs(), GetHamsterConfigs(), ENV_PRODUCTION, ENV_HAMSTER);
        EnsureSameSetOfConfigs(GetHamsterConfigs(), GetProductionConfigs(), ENV_HAMSTER, ENV_PRODUCTION);
    }

    Y_UNIT_TEST(EnsureRcAndProductionContainsTheSameSetOfScenarios) {
        EnsureSameSetOfConfigs(GetProductionConfigs(), GetRcConfigs(), ENV_PRODUCTION, ENV_RC);
        EnsureSameSetOfConfigs(GetRcConfigs(), GetProductionConfigs(), ENV_RC, ENV_PRODUCTION);
    }

    Y_UNIT_TEST(EnsureServerConfigsAreValid) {
        for (const auto& [env, _] : GetConfigs()) {
            const i32 argc = 5;
            const auto& configPath = GetServerConfigPath(env);
            const char* argv[argc] = {"", "-c", configPath.c_str(), "-p", "1"};
            LoadConfig(argc, argv);
        }
    }

    Y_UNIT_TEST(CheckProductionProps) {
        TStringBuilder error{};
        for (const auto& [env, configs] : GetConfigs()) {
            if (env == ENV_DEV) {
                continue;
            }
            for (const auto& [file, config] : configs) {
                const auto& handlers = config.GetHandlers();
                CHECK_FAILURE(handlers.GetBaseUrl().empty(), "Handlers.BaseUrl not set");

                NUri::TUri uri;
                CHECK_FAILURE(!NNetwork::TryParseUri(handlers.GetBaseUrl(), uri), "Failed to parse uri");
                CHECK_FAILURE(uri.GetHost() == "localhost" && uri.GetPort() > 86, "Using localhost in config");

                CHECK_FAILURE(config.GetResponsibles().GetAbcServices().size() == 0,
                    "No abc service provided, please add 'AbcServices' field to Responsibles");
                for (const auto& abc : config.GetResponsibles().GetAbcServices()) {
                    CHECK_FAILURE(abc.GetName() == "bassdevelopers",
                        "Please, do not use abc:bassdevelopers as a responsible sevice for your scenario");
                }
                for (const auto& frame : config.GetAcceptedFrames()) {
                    CHECK_FAILURE(!(frame.StartsWith("personal_assistant.") ||
                                    frame.StartsWith("alice.") ||
                                    frame.StartsWith("quasar.")),
                                  "Frames must be named like alice.* or personal_assistant.*, but " << frame << " found");
                }
                CHECK_FAILURE(config.GetAcceptedFrames().empty() && !config.GetAcceptsAnyUtterance(),
                              "Unable to found entry for this scenario, one of AcceptedFrames and AcceptsAnyUtterance "
                                  << "fields must be set");
            }
        }
        UNIT_ASSERT_C(error.empty(), error);
    }

    Y_UNIT_TEST(CheckHollywoodScenarios) {
        TStringBuilder error{};
        for (const auto& [env, configs] : GetConfigs()) {
            for (const auto& [file, config] : configs) {
                const auto& handlers = config.GetHandlers();
                if (handlers.GetBaseUrl().Contains("scenarios.hamster.alice.yandex.net") ||
                    handlers.GetBaseUrl().Contains("vins.alice.yandex.net"))
                {
                    CHECK_FAILURE(!handlers.GetOverrideHttpAdapterReqId(), "OverrideHttpAdapterReqId is not set for hollywood scenario");
                    CHECK_FAILURE(handlers.GetRequestType() == ERequestType::AppHostProxy, "Hollywood cannot have AppHostProxy RequestType");
                }
                CHECK_FAILURE(handlers.GetBaseUrl().Contains("hollywood.alice"), "hollywood.alice.yandex.net is no longer viable");
            }
        }
        UNIT_ASSERT_C(error.empty(), error);
    }

    Y_UNIT_TEST(CheckTransferringScenarios) {
        TStringBuilder error{};
        for (const auto& [env, configs] : GetConfigs()) {
            for (const auto& [file, config] : configs) {
                const auto& handlers = config.GetHandlers();
                if (handlers.GetIsTransferringToAppHostPure()) {
                    CHECK_FAILURE(handlers.GetRequestType() != ERequestType::AppHostProxy, "Only AppHostProxy scenarios can transfer to AppHostPure");
                    CHECK_FAILURE(handlers.GetGraphsPrefix().empty(), "Missing GraphsPrefix in TranferringToAppHostPure scenario");
                }
            }
        }
        UNIT_ASSERT_C(error.empty(), error);
    }

    Y_UNIT_TEST(CheckVinsScenarios) {
        TStringBuilder error{};
        for (const auto& [env, configs] : GetConfigs()) {
            for (const auto& [file, config] : configs) {
                const auto& handlers = config.GetHandlers();
                if (handlers.GetBaseUrl().Contains("vins-proxy.alice.yandex.net") || handlers.GetBaseUrl().Contains("vins-proxy.hamster.alice.yandex.net")) {
                    CHECK_FAILURE(handlers.GetOverrideHttpAdapterReqId(), "OverrideHttpAdapterReqId should not be set for vins scenario");
                    CHECK_FAILURE(handlers.GetRequestType() != ERequestType::AppHostProxy, "Vins should have AppHostProxy RequestType");
                    CHECK_FAILURE(handlers.GetBaseUrl().Contains("alice.yandex.net/vins"), "Vins scenarios path should start with /proto");
                }
                CHECK_FAILURE(handlers.GetBaseUrl().Contains("vins.hamster.alice.yandex.net"), "You shouldn't use vins.hamster.alice.yandex.net");
            }
        }
        UNIT_ASSERT_C(error.empty(), error);
    }

    Y_UNIT_TEST(CheckBaseUrl) {
        TStringBuilder error{};
        for (const auto& [env, configs] : GetConfigs()) {
            for (const auto& [file, config] : configs) {
                const auto& handlers = config.GetHandlers();
                CHECK_FAILURE(!handlers.GetBaseUrl().EndsWith('/'), "BaseUrl should end with a '/'");
                CHECK_FAILURE(handlers.GetRequestType() != ERequestType::AppHostProxy && handlers.GetRequestType() != ERequestType::AppHostPure, "Scenario should have RequestType");
            }
        }
        UNIT_ASSERT_C(error.empty(), error);
    }

    Y_UNIT_TEST(CheckDataSources) {
        TStringBuilder error{};
        for (const auto& [env, configs] : GetConfigs()) {
            for (const auto& [file, config] : configs) {
                for (const auto& dataSource : config.GetDataSources()) {
                    CHECK_FAILURE(EDataSourceType_Name(dataSource.GetType()).StartsWith("WEB_SEARCH") && config.GetHandlers().GetRequestType() != ERequestType::AppHostPure, "Scenarios with WEB_SEARCH dependencies should be AppHostPure");
                }
            }
        }
        UNIT_ASSERT_C(error.empty(), error);
    }

    Y_UNIT_TEST(TestCheckFailure) {
        TestConfigsCheckFailure<TScenarioConfig>();
    }

    Y_UNIT_TEST(TestConfigsHaveSameEnableStatus) {
        TestConfigsHaveSameEnableStatus(GetConfigs());
    }

    Y_UNIT_TEST(TestOnlyDevConfigsShouldBeDisabled) {
        TestOnlyDevConfigsShouldBeDisabled(GetProductionConfigs(), GetDevConfigs());
    }

    Y_UNIT_TEST(TestDescriptionIsProvidedInProductionConfigs) {
        TStringBuilder error{};
        const auto env = ENV_PRODUCTION;
        for (const auto& [file, config] : GetProductionConfigs()) {
            CHECK_FAILURE(config.GetDescription().empty(), "Description should be provided in production scenarios");
        }
        if (!error.empty()) {
            error << "Example: "
                  << R"(Description: "Сценарий <Нарисуй картину>: просим Алису что-нибудь нарисовать и получаем")"
                  << R"(изображение сгенерированное GANом, есть возможность получить рисунок на конкретную тему.)"
                  << R"(Примеры запросов: 'нарисуй картину', 'создай шедевр в стиле пикассо'."\n)";
        }
        UNIT_ASSERT_C(error.empty(), error);
    }

     Y_UNIT_TEST(CheckLocalhost) {
        THashSet<TString> productionConfigs{};
        for (const auto& [_, config] : GetProductionConfigs()) {
            productionConfigs.insert(config.GetName());
        }
        TStringBuilder error{};
        for (const auto& [env, configs] : GetConfigs()) {
            for (const auto& [file, config] : configs) {
                const auto& handlers = config.GetHandlers();
                if (env == ENV_DEV && !productionConfigs.count(config.GetName())) {
                    continue;
                }
                CHECK_FAILURE(handlers.GetBaseUrl().Contains("localhost"), "localhost isn't allowed for scenarios");
            }
        }
        UNIT_ASSERT_C(error.empty(), error);
    }

     Y_UNIT_TEST(CheckMementoUserConfigKeys) {
        static const auto mapping = NAlice::NMegamind::NImpl::LoadUserConfigsMapping();
        THashMap<TString, bool> statuses{};
        TStringBuilder error{};
        for (const auto& [env, configs] : GetConfigs()) {
            for (const auto& [file, config] : configs) {
                for (const auto& param : config.GetMementoUserConfigs()) {
                    CHECK_FAILURE(!mapping.contains(param.GetConfigKey()),
                                "Invalid memento key: "
                                    << ru::yandex::alice::memento::proto::EConfigKey_Name(param.GetConfigKey()));
                }
            }
        }
        UNIT_ASSERT_C(error.empty(), error);
    }

    Y_UNIT_TEST(TestScenarioAndFileNames) {
        TestConfigAndFileNames(GetConfigs());
    }

    Y_UNIT_TEST(TestConditionalDatasources) {
        for (const auto& [env, config] : GetConfigs()) {
            const i32 argc = 5;
            const auto& configPath = GetServerConfigPath(env);
            const char* argv[argc] = {"", "-c", configPath.c_str(), "-p", "1"};
            const auto serverConfig = LoadConfig(argc, argv);
            for (const auto& [scenarioName, scenarioGlobalConfig] : serverConfig.GetScenarios().GetConfigs()) {
                if (scenarioName == "TestScenario") {
                    continue;
                }
                for (const auto& cds : scenarioGlobalConfig.GetConditionalDataSources()) {
                    for (const auto& cond : cds.GetConditions()) {
                        if (cond.HasOnSemanticFrameCondition()) {
                            const auto& frameName = cond.GetOnSemanticFrameCondition().GetSemanticFrameName();
                            UNIT_ASSERT(IsIn(config.at(scenarioName+".pb.txt").GetAcceptedFrames(), frameName));
                        }
                    }
                }
            }
        }
    }

    Y_UNIT_TEST(ValidateUnknownEnumsInClassificationConfig) {
        for (const auto& [scenarioName, scenarioClfConfig] : GetClassificationConfig().GetScenarioClassificationConfigs()) {
            if (!scenarioClfConfig.HasFormulasDescriptionList()) {
                continue;
            }

            for (const TFormulaDescription& description : scenarioClfConfig.GetFormulasDescriptionList().GetFormulasDescription()) {
                UNIT_ASSERT(description.GetKey().GetLanguage() != L_UNK);
                UNIT_ASSERT(description.GetKey().GetClassificationStage() != ECS_UNKNOWN);
                UNIT_ASSERT(description.GetKey().GetClientType() != ECT_UNKNOWN);
            }
        }
    }

    Y_UNIT_TEST(CheckCallbacks) {
        for (const auto& [env, configs] : GetConfigs()) {
            for (const auto& [file, config] : configs) {
                for (const auto& callbackName : config.GetAcceptedTypedCallbacks()) {
                    UNIT_ASSERT_C(IsExistingTypedCallback(callbackName),
                                  "There is no typed callback with name = \"" + callbackName + "\"");
                }
            }
        }
    }
}

} // namespace NAlice::NMegamind

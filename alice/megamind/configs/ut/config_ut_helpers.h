#pragma once

#include <alice/megamind/library/config/config.h>
#include <alice/megamind/library/config/scenario_protos/combinator_config.pb.h>
#include <alice/megamind/library/config/scenario_protos/config.pb.h>

#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/folder/path.h>
#include <util/string/builder.h>

namespace NAlice::NMegamind::NTesting {

using TScenarioConfigs = THashMap<TString, TScenarioConfig>;
using TCombinatorConfigs = THashMap<TString, TCombinatorConfigProto>;

const TString ENV_PRODUCTION = "production";
const TString ENV_RC = "rc";
const TString ENV_HAMSTER = "hamster";
const TString ENV_DEV = "dev";

TFsPath GetConfigPath(const TString& env);

TFsPath GetCombinatorConfigPath(const TString& env);

TFsPath GetScenarioConfigPath(const TString& env);

TFsPath GetServerConfigPath(const TString& env);

template <typename TConfig>
TString GetOwners(const TConfig& config) {
    TStringBuilder owners{};
    for (const auto& login : config.GetResponsibles().GetLogins()) {
        if (!owners.empty()) {
            owners << ", ";
        }
        owners << "@" << login;
    }
    for (const auto& abc : config.GetResponsibles().GetAbcServices()) {
        if (!owners.empty()) {
            owners << ", ";
        }
        owners << "@abc:" << abc.GetName();
    }
    return owners;
}

#define CHECK_FAILURE(failed, hint)                                                                                   \
    do {                                                                                                              \
        if (failed) {                                                                                                 \
            error << "[" << env << "] " << file << ": " << hint << " | Owners: " << GetOwners(config) << "\n";        \
        }                                                                                                             \
    } while (false)

template <typename TConfigs>
void EnsureSameSetOfConfigs(const TConfigs& lhs, const TConfigs& rhs, const TString& lhsEnv,
                            const TString& rhsEnv) {
    for (const auto& [file, config] : lhs) {
        const auto rhsConfigInfo = rhs.find(file);
        UNIT_ASSERT_C(rhsConfigInfo != rhs.end(), "Unable to find '" << file << "' in " << rhsEnv << " env");
        const auto lhsName = config.GetName();
        const auto rhsName = rhsConfigInfo->second.GetName();
        UNIT_ASSERT_VALUES_EQUAL_C(lhsName, rhsName,
                                   "(" << lhsEnv << " config name) != (" << rhsEnv << " config name) : ("
                                       << lhsName << ") != (" << rhsName << ") in file: " << file);
    }
};

template <typename TConfig>
void TestConfigsCheckFailure() {
    TStringBuilder error{};
    const auto file = "File";
    const auto config = []() {
        TConfig config{};
        auto* responsibles = config.MutableResponsibles();
        *responsibles->AddLogins() = "Owner";
        responsibles->AddAbcServices()->SetName("service");
        return config;
    }();
    const auto env = "Env";
    CHECK_FAILURE(true, "Hint");
    UNIT_ASSERT_VALUES_EQUAL(ToString(error), "[Env] File: Hint | Owners: @Owner, @abc:service\n");
}

template <typename TConfigs>
void TestConfigsHaveSameEnableStatus(const TConfigs& configsMap) {
    THashMap<TString, bool> statuses{};
    TStringBuilder error{};
    for (const auto& [env, configs] : configsMap) {
        for (const auto& [file, config] : configs) {
            if (!statuses.contains(config.GetName())) {
                statuses[config.GetName()] = config.GetEnabled();
            }
            CHECK_FAILURE(
                statuses.at(config.GetName()) != config.GetEnabled(),
                "Combinators's and scenario's configs should have the same Enabled property value across all environments");
        }
    }
    UNIT_ASSERT_C(error.empty(), error);
}

template <typename TConfigs>
void TestOnlyDevConfigsShouldBeDisabled(const TConfigs& productionConfigs, const TConfigs& devConfigs) {
    THashSet<TString> names{};
    TStringBuilder error{};
    for (const auto& [_, config] : productionConfigs) {
        names.insert(config.GetName());
    }
    const auto env = ENV_DEV;
    for (const auto& [file, config] : devConfigs) {
        CHECK_FAILURE(!names.contains(config.GetName()) && config.GetEnabled(),
                      "Combinators and scenarios under development should be disabled");
    }
    UNIT_ASSERT_C(error.empty(), error);
}

template <typename TConfigs>
void TestConfigAndFileNames(const TConfigs& configsMap) {
    TStringBuilder error{};
    for (const auto& [env, configs] : configsMap) {
        for (const auto& [file, config] : configs) {
            for (const auto c : config.GetName()) {
                CHECK_FAILURE(std::isalnum(c) == 0, "Combinator and scenario name should consist of english letters and digits");
            }
            CHECK_FAILURE(config.GetName() + ".pb.txt" != file, "file and combinator/scenario names should match ");
        }
    }
    UNIT_ASSERT_C(error.empty(), error);
}

bool IsExistingTypedCallback(const TString& typedCallbackName);

} // NAlice::NMegamind::NTesting

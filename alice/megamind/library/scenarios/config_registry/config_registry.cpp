#include "config_registry.h"
#include "config_validator.h"

#include <alice/megamind/library/util/fs.h>

#include <library/cpp/protobuf/util/pb_io.h>

#include <util/generic/hash.h>
#include <util/generic/yexception.h>

namespace NAlice {

namespace {

template <typename TConfig>
THashMap<TString, TConfig> LoadConfigs(const TString& folderPath, bool validateConfigs,
                                       std::function<void(const TConfig&)> validateFn) {
    THashMap<TString, TConfig> configs{};
    for (const auto& path : NMegamind::ListFilesInDirectory(folderPath)) {
        if (!path.GetName().EndsWith(".pb.txt")) {
            continue;
        }
        configs[path.GetName()] = ParseConfig<TConfig>(TFileInput(path).ReadAll());
        if (validateConfigs) {
            try {
                validateFn(configs[path.GetName()]);
            } catch (...) {
                ythrow yexception() << "Error while loading scenario/combinator config '" << path << "'\n"
                                    << CurrentExceptionMessage();
            }
        }
    }
    return configs;
}

} // namespace

THashMap<TString, TScenarioConfig> LoadScenarioConfigs(const TString& folderPath, bool validateConfigs) {
    return LoadConfigs<TScenarioConfig>(folderPath, validateConfigs, NMegamind::ValidateScenarioConfig);
}

THashMap<TString, NMegamind::TCombinatorConfigProto> LoadCombinatorConfigs(const TString& folderPath, bool validateConfigs) {
    return LoadConfigs<NMegamind::TCombinatorConfigProto>(folderPath, validateConfigs, NMegamind::ValidateCombinatorConfig);
}

// TScenarioConfigRegistry -----------------------------------------------------
void TScenarioConfigRegistry::AddScenarioConfig(const TScenarioConfig& config) {
    const auto& [_, ok] = ScenarioConfigs.emplace(config.GetName(), config);
    Y_ENSURE(ok, "Duplicate config for scenario " << config.GetName());
}

const THashMap<TString, TScenarioConfig>& TScenarioConfigRegistry::GetScenarioConfigs() const {
    return ScenarioConfigs;
}

const TScenarioConfig& TScenarioConfigRegistry::GetScenarioConfig(const TString& scenarioName) const {
    if (const auto config = ScenarioConfigs.FindPtr(scenarioName)) {
        return *config;
    }
    return TScenarioConfig::default_instance();
}

// TCombinatorConfigRegistry -----------------------------------------------------
void TCombinatorConfigRegistry::AddCombinatorConfig(const TCombinatorConfig& config) {
    const auto& [_, ok] = CombinatorConfigs.emplace(config.GetName(), config);
    Y_ENSURE(ok, "Duplicate config for combinator " << config.GetName());
}

const THashMap<TString, TCombinatorConfig>& TCombinatorConfigRegistry::GetCombinatorConfigs() const {
    return CombinatorConfigs;
}

const TCombinatorConfig& TCombinatorConfigRegistry::GetCombinatorConfig(const TString& combinatorName) const {
    if (const auto config = CombinatorConfigs.FindPtr(combinatorName)) {
        return *config;
    }
    return TCombinatorConfig::GetDefaultInstance();
}

} // namespace NAlice

#include "config_helpers.h"

#include <library/cpp/getoptpb/getoptpb.h>

namespace NAlice::NModalSessionMapper {

TScenarioMaxTurns ScenarioMaxTurnsFromConfig(const TConfig& config) {
    TScenarioMaxTurns result;

    for (const auto& [scenarioName, scenarioConfig] : config.GetScenarios().GetConfigs()) {
        if (scenarioConfig.GetDialogManagerParams().HasMaxActivityTurns()) {
            result[scenarioName] = scenarioConfig.GetDialogManagerParams().GetMaxActivityTurns();
        }
    }

    return result;
}

TScenarioMaxTurns ScenarioMaxTurnsFromCmdline(const TString& configPath) {
    const char* configArgv[] = {"unused", "-c", configPath.c_str()};

    TString errorMsg;
    TConfig config;
    NGetoptPb::TGetoptPbSettings settings{};

    NGetoptPb::TGetoptPb configPathOpt(settings);
    configPathOpt.AddOptions(config);
    Y_ENSURE(configPathOpt.ParseArgs(Y_ARRAY_SIZE(configArgv), configArgv, config, errorMsg),
             "Can not parse command line options and/or prototext config, explanation: " << errorMsg);
    return ScenarioMaxTurnsFromConfig(config);
}

} // namespace NAlice::NModalSessionMapper

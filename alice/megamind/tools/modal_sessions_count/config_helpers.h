#pragma once

#include <alice/megamind/library/config/config.h>

#include <util/generic/hash.h>
#include <util/generic/string.h>

namespace NAlice::NModalSessionMapper {

using TScenarioMaxTurns = THashMap<TString, i32>;

TScenarioMaxTurns ScenarioMaxTurnsFromConfig(const NAlice::TConfig& config);
TScenarioMaxTurns ScenarioMaxTurnsFromCmdline(const TString& configPath);

} // namespace NAlice::NModalSessionMapper

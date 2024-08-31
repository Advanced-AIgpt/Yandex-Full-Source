#pragma once

#include <library/cpp/json/json_value.h>

namespace NAlice {

[[nodiscard]] bool UpdateVinsWizardRules(NJson::TJsonValue& rules, const NJson::TJsonValue& rawWizardResponse);

} // namespace NAlice

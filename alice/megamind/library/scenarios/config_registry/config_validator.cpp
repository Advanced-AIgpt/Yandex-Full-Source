#include "config_validator.h"

#include <util/string/ascii.h>

namespace NAlice::NMegamind {

namespace {

constexpr TStringBuf NAME_HINT = "Scenario/Combinator name should be written in PascalCase (^[A-Z][\\w\\d]{2,}$)";

void ValidateName(const TString& name) {
    Y_ENSURE(!name.empty(), "Scenario/Combinator name can't be empty – " << NAME_HINT);
    Y_ENSURE(name.size() > 2, "Scenario/Combinator name should contain at least 3 characters – " << NAME_HINT);
    Y_ENSURE(IsAsciiUpper(name[0]), NAME_HINT);
    for (char c : name) {
        Y_ENSURE(IsAsciiAlnum(c), "Invalid character '" << c << "' – " << NAME_HINT);
    }
}

} // namespace

void ValidateScenarioConfig(const TScenarioConfig& config) {
    ValidateName(config.GetName());
}

void ValidateCombinatorConfig(const TCombinatorConfigProto& config) {
    ValidateName(config.GetName());
}

} // namespace NAlice::NMegamind

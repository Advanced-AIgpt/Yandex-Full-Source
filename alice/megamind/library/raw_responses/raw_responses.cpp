#include "raw_responses.h"

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

#include <library/cpp/json/json_writer.h>

namespace NAlice {

namespace {

constexpr TStringBuf RULES_PATH = "rules";

const TVector<TString> WIZARD_VINS_RULES_WHITELIST = {
    "AliceAnaphoraSubstitutor",
    "AliceTypeParserTime",
    "CustomEntities",
    "Date",
    "DirtyLang",
    "EntityFinder",
    "EntitySearch",
    "ExternalMarkup",
    "Fio",
    "GeoAddr",
    "Granet",
    "IsNav",
    "MusicFeatures",
    "Wares"
};

} // namespace

bool UpdateVinsWizardRules(NJson::TJsonValue& rules, const NJson::TJsonValue& rawWizardResponse) {
    const NJson::TJsonValue& rawResponseRules = rawWizardResponse[RULES_PATH];
    if (!rawResponseRules.IsMap()) {
        return false;
    }
    rules.SetType(NJson::JSON_MAP);
    for (const auto& ruleName : WIZARD_VINS_RULES_WHITELIST) {
        if (rawResponseRules.Has(ruleName)) {
            rules[ruleName].SetValue(rawResponseRules[ruleName]);
        }
    }
    return true;
}

} // namespace NAlice

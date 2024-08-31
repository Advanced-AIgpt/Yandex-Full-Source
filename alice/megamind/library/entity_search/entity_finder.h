#pragma once

#include <library/cpp/json/json_value.h>

#include <util/generic/string.h>

namespace NAlice {

namespace NEntitySearch::NEntityFinder {

TString ExtractEntityId(TStringBuf winner);

TString GetEntitiesString(const NJson::TJsonValue& vinsWizardResponse);

} // namespace NEntitySearch::NEntityFinder

} // namespace NAlice

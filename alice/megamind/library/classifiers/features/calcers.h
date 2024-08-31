#pragma once

#include <alice/megamind/library/scenarios/features/features.h>

#include <util/generic/strbuf.h>

class TFactorStorage;

namespace NAlice {

class TWizardResponse;
class TSearchResponse;

void FillQueryFactors(const TWizardResponse& response, TFactorStorage& storage);
void FillScenarioQueryFactors(const TWizardResponse& response, TFactorStorage& storage);

void FillSearchFactors(const TStringBuf utterance, const TSearchResponse& response, TFactorStorage& storage);

void FillScenarioFactors(const NMegamind::TScenarioFeatures& features, TFactorStorage& storage);

void FillNluFactors(const TWizardResponse& response, TFactorStorage& storage);

} // namespace NAlice

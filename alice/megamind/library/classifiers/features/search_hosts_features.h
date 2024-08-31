#pragma once

#include <kernel/alice/search_scenario_factors_info/factors_gen.h>

#include <util/generic/fwd.h>

class TFactorView;

namespace NSc {
class TValue;
}

namespace NAlice {

namespace NScenarios {
class TWebSearchDoc;
}

namespace NResponseSimilarity {
class TSimilarity;
}

struct THostsPositions {
    TVector<ui32> Music;
    TVector<ui32> Video;
    THashMap<TStringBuf, TVector<ui32>> PerHost;
};

void ProcessWebResult(
    const NSc::TValue& doc,
    const ui32 position,
    const TStringBuf utterance,
    THostsPositions& hostsPositions,
    TVector<NResponseSimilarity::TSimilarity>& titleSimilarities,
    TVector<NResponseSimilarity::TSimilarity>& headlineSimilarities
);
void ProcessWizardResult(
    const NSc::TValue& doc,
    const float dcg,
    const TFactorView view
);

void ProcessDocWebResult(
    const NScenarios::TWebSearchDoc& doc,
    const ui32 position,
    const TStringBuf utterance,
    THostsPositions& hostsPositions,
    TVector<NResponseSimilarity::TSimilarity>& titleSimilarities,
    TVector<NResponseSimilarity::TSimilarity>& headlineSimilarities
);
void ProcessDocWizardResult(
    const NScenarios::TWebSearchDoc& doc,
    const float dcg,
    const TFactorView view
);
void ProcessDocSnippetResult(
    const NScenarios::TWebSearchDoc& doc,
    const float dcg,
    const TFactorView view
);

void FillHostDCG(
    const THostsPositions& hostsPositions,
    const TFactorView view
);

} // namespace NAlice

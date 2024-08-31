#pragma once

#include <alice/megamind/library/kv_saas/response.h>

#include <alice/library/client/client_features.h>

class TFactorStorage;

namespace NAlice {

void FillQueryTokensFactors(const NKvSaaS::TTokensStatsResponse& tokensStatsResponse, const TClientFeatures& clientFeatures, TFactorStorage& storage);

} // namespace NAlice

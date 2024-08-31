#pragma once

#include <alice/bass/forms/context/fwd.h>
#include <alice/bass/util/error.h>
#include <alice/library/analytics/interfaces/analytics_info_builder.h>
#include <alice/library/client/client_features.h>
#include <library/cpp/scheme/scheme.h>
#include <util/datetime/base.h>
#include <util/generic/string.h>

namespace NBASS::NMusic {

struct TCheckAndSearchMusicParams {
    bool HasSearch = false;
    bool RichTracks = true;
    TString TextQuery;
    TContext& Ctx; // this is evil
    TInstant RequestStartTime;
    NAlice::NScenarios::IAnalyticsInfoBuilder& AnalyticsInfoBuilder;
    const NAlice::TClientFeatures& ClientFeatures;
};

struct TCheckAndSearchMusicResult {
    TResultValue Result;
    NSc::TValue WebAnswer = NSc::TValue::Null();
    NSc::TValue FeaturesData = NSc::TValue::Null();
    TMaybe<i64> TotalMilliseconds;
    TMaybe<i64> WebSearchMilliseconds;
    TMaybe<i64> CatalogMilliseconds;

    struct TCatalogTiming {
        TString Path;
        TString Type;
        i64 TimeMilliseconds = 0;
    };

    TVector<TCatalogTiming> CatalogTimings;
};

[[nodiscard]] TCheckAndSearchMusicResult CheckAndSearchMusic(const TCheckAndSearchMusicParams& params);

} // namespace NBASS::NMusic

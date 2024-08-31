#include "intents.h"

#include <util/generic/hash_set.h>
#include <util/generic/strbuf.h>

namespace NAlice {

namespace {

const THashSet<TStringBuf> SPECIAL_PLAYLISTS = {
    "Громкие новинки месяца",
    "Дежавю",
    "Плейлист дня",
    "Премьера",
    "Чарт Яндекс.Музыки",
};

NYT::TNode GetAnalyticsInfo(const NYT::TNode& megamindAnalyticsInfo) {
    if (megamindAnalyticsInfo.HasValue() && megamindAnalyticsInfo.HasKey("analytics_info")) {
        return megamindAnalyticsInfo.At("analytics_info");
    }
    return NYT::TNode{};
}

NYT::TNode GetIntentFromScenarioAnalyticsInfo(const NYT::TNode& analyticsInfo) {
    NYT::TNode intent;
    for (const auto& info : analyticsInfo.AsMap()) {
        if (info.second.HasKey("scenario_analytics_info")
            && info.second.At("scenario_analytics_info").HasKey("intent")) {
            intent = info.second.At("scenario_analytics_info").At("intent");
        }
        return intent;
    }
    return intent;
}

NYT::TNode GetIntentFromSemanticFrame(const NYT::TNode& analyticsInfo) {
    NYT::TNode intent;
    for (const auto& info : analyticsInfo.AsMap()) {
        if (info.second.HasKey("semantic_frame")
            && info.second.At("semantic_frame").HasKey("name")) {
            intent = info.second.At("semantic_frame").At("name");
        }
        return intent;
    }
    return intent;
}

} // namespace

TString GetIntentName(const NYT::TNode& megamindAnalyticsInfo, const TString& formName) {
    const NYT::TNode analyticsInfo = GetAnalyticsInfo(megamindAnalyticsInfo);
    NYT::TNode intent;

    if (analyticsInfo.HasValue() && analyticsInfo.IsMap()) {
        intent = GetIntentFromScenarioAnalyticsInfo(analyticsInfo);
        if (!intent.HasValue()) {
            intent = GetIntentFromSemanticFrame(analyticsInfo);
        }
    }

    return (intent.IsString()) ? intent.AsString() : formName;
}

} // namespace NAlice

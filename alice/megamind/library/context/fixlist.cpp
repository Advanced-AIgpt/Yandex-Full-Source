#include "fixlist.h"

namespace NAlice {

namespace {

constexpr TStringBuf GENERAL_FIXLIST = "general_fixlist";

THashSet<TString> ParseGeneralFixlistMatches(const ::NBg::NProto::TAliceFixlistResult& aliceFixlistResult) {
    const auto it = aliceFixlistResult.GetMatches().find(GENERAL_FIXLIST);
    if (it == aliceFixlistResult.GetMatches().end()) {
        return THashSet<TString>();
    }
    const auto& intents = it->second.GetIntents();
    return THashSet<TString>{intents.cbegin(), intents.cend()};
}

THashMap<TString, THashSet<TString>> ParseGeneralFixlistSupportedFeaturesMatches(const THashSet<TString>& generalFixlistMatches) {
    auto result = THashMap<TString, THashSet<TString>>();
    for (const TStringBuf match : generalFixlistMatches) {
        TStringBuf fixlistScenario;
        TStringBuf fixlistSupportedFeature;
        if (match.TryRSplit(':', fixlistScenario, fixlistSupportedFeature)) {
            result[fixlistScenario].emplace(fixlistSupportedFeature);
        }
    }
    return result;
}

} // namespace

TFixlist::TFixlist(const ::NBg::NProto::TAliceFixlistResult& aliceFixlistResult)
    : HasAnyMatch_(!aliceFixlistResult.GetMatches().empty())
    , GeneralFixlistMatches_(ParseGeneralFixlistMatches(aliceFixlistResult))
    , GeneralFixlistSupportedFeaturesMatches_(ParseGeneralFixlistSupportedFeaturesMatches(GeneralFixlistMatches_))
{
}

bool TFixlist::IsRequestAllowedForScenario(TStringBuf scenarioName) const {
    if (!HasAnyMatch_) {
        return false;
    }
    if (GeneralFixlistMatches_.empty()) {
        return true;
    }
    return GeneralFixlistMatches_.contains(scenarioName);
}

const THashSet<TString>& TFixlist::GetMatches() const {
    return GeneralFixlistMatches_;
}

const THashMap<TString, THashSet<TString>>& TFixlist::GetSupportedFeaturesMatches() const {
    return GeneralFixlistSupportedFeaturesMatches_;
}


} // namespace NAlice

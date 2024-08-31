#pragma once

#include <search/begemot/rules/alice/fixlist/proto/alice_fixlist.pb.h>

#include <util/generic/hash_set.h>
#include <util/generic/hash.h>
#include <util/generic/string.h>

namespace NAlice {

class TFixlist {
public:
    TFixlist() = default;

    explicit TFixlist(const ::NBg::NProto::TAliceFixlistResult& aliceFixlistResult);

    bool IsRequestAllowedForScenario(TStringBuf scenarioName) const;

    const THashSet<TString>& GetMatches() const;
    const THashMap<TString, THashSet<TString>>& GetSupportedFeaturesMatches() const;

private:
    bool HasAnyMatch_ = false;
    THashSet<TString> GeneralFixlistMatches_;
    THashMap<TString, THashSet<TString>> GeneralFixlistSupportedFeaturesMatches_;
};

} // namespace NAlice

#pragma once

#include <alice/megamind/protos/analytics/combinators/combinator_analytics_info.pb.h>
#include <alice/hollywood/library/combinators/request/request.h>

#include <util/generic/maybe.h>

namespace NAlice::NHollywood::NCombinators {

class TCombinatorAnalyticsInfoBuilder : public NNonCopyable::TMoveOnly {

    using TCombinatorAnalyticsInfo = ::NAlice::NCombinators::TCombinatorAnalyticsInfo;

public:
    TCombinatorAnalyticsInfoBuilder(const TCombinatorRequestWrapper& request) {
        for (const auto& [name, response] : request.GetScenarioRunResponses()) {
            auto& incomingScenarioInfo = (*Proto_.MutableIncomingScenarioInfos())[name];
            incomingScenarioInfo.SetIsIrrelevant(response.GetFeatures().GetIsIrrelevant());
        }
        // TODO analytics for continue scenario responses
    }

    TCombinatorAnalyticsInfoBuilder& SetCombinatorProductName(const TString& combinatorProductName) {
        Proto_.SetCombinatorProductName(combinatorProductName);
        return *this;
    }

    TCombinatorAnalyticsInfo MoveProto() && {
        return std::move(Proto_);
    }

private:
    TCombinatorAnalyticsInfo Proto_;
};

} // namespace NAlice::NHollywood::NCombinators

#pragma once

#include <alice/megamind/protos/analytics/combinators/combinator_analytics_info.pb.h>
#include <alice/megamind/protos/scenarios/action_space.pb.h>
#include <alice/megamind/protos/scenarios/combinator_response.pb.h>
#include <alice/megamind/protos/scenarios/directives.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood::NCombinators {

class TCombinatorRunResponseBuilder : public NNonCopyable::TMoveOnly {
public:

    TCombinatorRunResponseBuilder()
        : Response_()
    {
    }

    TCombinatorRunResponseBuilder& AddUsedScenario(const TString& scenario) {
        Response_.AddUsedScenarios(scenario);
        return *this;
    }

    TCombinatorRunResponseBuilder& ScenarioResponse(const TScenarioRunResponse& scenarioRunResponse) {
        *Response_.MutableResponse() = scenarioRunResponse;
        return *this;
    }

    TCombinatorRunResponseBuilder& ScenarioResponse(TScenarioRunResponse&& scenarioRunResponse) {
        *Response_.MutableResponse() = std::move(scenarioRunResponse);
        return *this;
    }

    TCombinatorRunResponseBuilder& AnalyticsInfo(::NAlice::NCombinators::TCombinatorAnalyticsInfo&& analyticsInfo) {
        *Response_.MutableCombinatorsAnalyticsInfo() = std::move(analyticsInfo);
        return *this;
    }

    TCombinatorRunResponseBuilder& AddDirective(TDirective&& directive) {
        *Response_.MutableResponse()->MutableResponseBody()->MutableLayout()->AddDirectives() = std::move(directive);
        return *this;
    }

    TCombinatorRunResponseBuilder& AddServerDirective(TServerDirective&& serverDirective) {
        *Response_.MutableResponse()->MutableResponseBody()->AddServerDirectives() = std::move(serverDirective);
        return *this;
    }

    TCombinatorRunResponseBuilder& AddActionSpace(const TString& actionSpaceId, const TActionSpace& actionSpace) {
        (*Response_.MutableResponse()->MutableResponseBody()->MutableActionSpaces())[actionSpaceId] = actionSpace;
        return *this;
    }

    TCombinatorResponse MoveProto() && {
        return std::move(Response_);
    }

private:
    TCombinatorResponse Response_;
};

} // namespace NAlice::NHollywood::NCombinators

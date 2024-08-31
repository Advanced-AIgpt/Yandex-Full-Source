#pragma once

#include "response_builder.h"

#include <alice/megamind/protos/common/app_type.pb.h>
#include <alice/megamind/protos/scenarios/directives.pb.h>
#include <alice/megamind/protos/scenarios/push.pb.h>

#include <util/generic/vector.h>

namespace NAlice::NHollywood {

constexpr ui64 DEFAULT_PUSH_TTL = 180u;

class TPushDirectiveBuilder {
public:
    // All necessary fields are filled in constructor
    TPushDirectiveBuilder(const TString& title,
                          const TString& text,
                          const TString& url,
                          const TString& tag);

    TPushDirectiveBuilder& SetTtlSeconds(ui64 ttl);

    // For when it is not equal to tag
    TPushDirectiveBuilder& SetPushId(const TString& id);

    // Doc: https://doc.yandex-team.ru/sup-api/concepts/push-create/index.html
    // Values: https://sup.yandex.net/throttle-policies
    TPushDirectiveBuilder& SetThrottlePolicy(const TString& policy);

    TPushDirectiveBuilder& SetAppTypes(const TVector<NAlice::EAppType>& appTypes);

    TPushDirectiveBuilder& SetCardImageUrl(const TString& url);

    TPushDirectiveBuilder& SetCardDateRange(const TString& from, const TString& to);

    TPushDirectiveBuilder& RemoveExistingCards();

    TPushDirectiveBuilder& OverridePushSettings(const NScenarios::TSendPushDirective::TCommon& settings);

    TPushDirectiveBuilder& OverrideCardSettings(const NScenarios::TSendPushDirective::TCommon& settings);

    TPushDirectiveBuilder& SetAnalyticsAction(TString id, TString name, TString description);

    void BuildTo(TResponseBodyBuilder&);
    void BuildTo(NHollywoodFw::TRender&);

private:
    NScenarios::TSendPushDirective Push;
    NScenarios::TAnalyticsInfo::TAction AnalyticsAction;
};

} // namespace NAlice::NHollywood

#pragma once

#include <alice/megamind/library/request/request.h>

#include <alice/megamind/protos/analytics/analytics_info.pb.h>
#include <alice/megamind/protos/analytics/combinators/combinator_analytics_info.pb.h>
#include <alice/megamind/protos/analytics/megamind_analytics_info.pb.h>
#include <alice/megamind/protos/analytics/modifiers/analytics_info.pb.h>
#include <alice/megamind/protos/analytics/modifiers/proactivity/postroll.pb.h>
#include <alice/megamind/protos/analytics/user_info.pb.h>
#include <alice/megamind/protos/common/atm.pb.h>
#include <alice/megamind/protos/common/iot.pb.h>
#include <alice/megamind/protos/modifiers/modifier_response.pb.h>
#include <alice/megamind/protos/modifiers/modifiers.pb.h>

#include <alice/library/json/json.h>

#include <util/generic/string.h>

namespace NAlice::NMegamind {

class TMegamindAnalyticsInfoBuilder {

    using TModifierAnalyticsInfo = TModifierResponse_TAnalyticsInfo;

public:
    TMegamindAnalyticsInfoBuilder() = default;
    ~TMegamindAnalyticsInfoBuilder() = default;

    TMegamindAnalyticsInfoBuilder& SetOriginalUtterance(const TString& utterance);
    TMegamindAnalyticsInfoBuilder& SetChosenUtterance(const TString& utterance);
    TMegamindAnalyticsInfoBuilder& SetShownUtterance(TString utterance);

    TMegamindAnalyticsInfoBuilder& AddUserInfo(const TString& scenarioName, const TUserInfo& userInfo);
    TMegamindAnalyticsInfoBuilder& AddAnalyticsInfo(const TString& scenarioName, const TAnalyticsInfo& analyticsInfo);
    TMegamindAnalyticsInfoBuilder& AddScenarioTimings(const TString& scenarioName,
                                                      const TAnalyticsInfo& analyticsInfo);
    TMegamindAnalyticsInfoBuilder& SetWinnerScenarioName(const TString& scenarioName);

    TMegamindAnalyticsInfoBuilder& AddTunnellerRawResponse(const TString& scenarioName,
                                                           const TString& tunnellerResponse);
    TMegamindAnalyticsInfoBuilder& CopyTunnellerRawResponses(const TString& scenarioName,
                                                             const TAnalyticsInfo& analyticsInfo);

    TMegamindAnalyticsInfoBuilder& SetModifiersInfo(const TModifiersInfo& modifiersInfo);
    TMegamindAnalyticsInfoBuilder&
    SetDeviceStateActions(const google::protobuf::Map<TString, TDeviceStateAction>& actions);

    TMegamindAnalyticsInfoBuilder& SetRecognizedScenarioAction(const TString& parentRequestId, const TString& actionId,
                                                               const TString& parentProductScenarioName);
    TMegamindAnalyticsInfoBuilder& SetRecognizedDeviceAction(const TString& actionId);
    TMegamindAnalyticsInfoBuilder& AddRecognizedSpaceAction(const TString& recognizedFrame,
                                                            const TAnalyticsTrackingModule& atm);

    TMegamindAnalyticsInfoBuilder& SetPreClassifyDuration(ui64 duration);
    TMegamindAnalyticsInfoBuilder& SetPostClassifyDuration(ui64 duration);
    TMegamindAnalyticsInfoBuilder& AddServiceSourceTimings(const TString& sourceName,
                                                           ui64 duration);

    TMegamindAnalyticsInfoBuilder& SetIoTUserInfo(const TIoTUserInfo& ioTUserInfo);
    TMegamindAnalyticsInfoBuilder& SetUserProfile(const TUserProfile& userProfile);

    TMegamindAnalyticsInfoBuilder& SetParentProductScenarioName(const TString& productScenario);
    TMegamindAnalyticsInfoBuilder& SetLocation(const TRequest::TLocation& location);

    TMegamindAnalyticsInfoBuilder& SetModifierAnalyticsInfo(const TModifierAnalyticsInfo& modifierAnalyticsInfo);
    TMegamindAnalyticsInfoBuilder& SetModifiersAnalyticsInfo(const NModifiers::TAnalyticsInfo& modifiersAnalyticsInfo);

    TMegamindAnalyticsInfoBuilder& SetPostroll(const NModifiers::NProactivity::TPostroll& postroll);

    TMegamindAnalyticsInfoBuilder& SetCombinatorAnalyticsInfo(
        const NCombinators::TCombinatorAnalyticsInfo& combinatorAnalyticsInfo);

    const TMegamindAnalyticsInfo& BuildProto() const;
    NJson::TJsonValue BuildJson() const;

    template <class TProto>
    TMegamindAnalyticsInfoBuilder& CopyFromProto(TProto&& proto);

private:
    TMegamindAnalyticsInfo MegamindAnalyticsInfo;
};

template <class TProto>
TMegamindAnalyticsInfoBuilder& TMegamindAnalyticsInfoBuilder::CopyFromProto(TProto&& proto) {
    MegamindAnalyticsInfo = std::forward<TMegamindAnalyticsInfo>(proto);
    return *this;
}

} // namespace NAlice::NMegamind

#pragma once

#include <alice/library/analytics/interfaces/analytics_info_builder.h>
#include <alice/megamind/protos/analytics/analytics_info.pb.h>
#include <alice/megamind/protos/analytics/user_info.pb.h>
#include <alice/megamind/protos/common/frame.pb.h>
#include <alice/megamind/protos/scenarios/user_info.pb.h>

#include <util/generic/string.h>

namespace NAlice::NMegamind {

class TAnalyticsInfoBuilder {
public:
    using TScenarioProto = NScenarios::TAnalyticsInfo;

    TAnalyticsInfoBuilder() = default;
    ~TAnalyticsInfoBuilder() = default;

    THolder<NScenarios::IAnalyticsInfoBuilder> CreateScenarioAnalyticsInfoBuilder();
    TAnalyticsInfoBuilder& CopyFrom(const TScenarioProto& scenarioProto);
    TAnalyticsInfoBuilder& Update(const TScenarioProto& scenarioProto);

    TAnalyticsInfoBuilder& SetVersion(const TString& version);
    TAnalyticsInfoBuilder& SetSemanticFrame(const TSemanticFrame& frame);
    TAnalyticsInfoBuilder& SetFrameActions(const google::protobuf::Map<TString, NScenarios::TFrameAction>& actions);
    TAnalyticsInfoBuilder& SetMatchedSemanticFrames(const TVector<TSemanticFrame>& semanticFrames);
    TAnalyticsInfoBuilder& SetParentRequestId(const TString& requestId);
    TAnalyticsInfoBuilder& SetParentProductScenarioName(const TString& productScenarioName);

    TAnalyticsInfo Build();

private:
    TAnalyticsInfo AnalyticsInfo;
};

class TUserInfoBuilder {
public:
    using TScenarioProto = NScenarios::TUserInfo;

    TUserInfoBuilder() = default;
    ~TUserInfoBuilder() = default;

    THolder<NScenarios::IUserInfoBuilder> CreateScenarioUserInfoBuilder();
    TUserInfoBuilder& CopyFrom(const TScenarioProto& scenarioProto);

    THolder<TUserInfo> Build() &&;

private:
    THolder<TUserInfo> UserInfo;
};

} // namespace NAlice::NMegamind

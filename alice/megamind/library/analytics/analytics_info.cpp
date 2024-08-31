#include "analytics_info.h"

#include <alice/megamind/protos/scenarios/response.pb.h>
#include <alice/library/analytics/scenario/builder.h>

namespace NAlice::NMegamind {

// TAnalyticsInfoBuilder -------------------------------------------------------
THolder<NScenarios::IAnalyticsInfoBuilder> TAnalyticsInfoBuilder::CreateScenarioAnalyticsInfoBuilder() {
    return MakeHolder<NScenarios::TAnalyticsInfoBuilder>(*AnalyticsInfo.MutableScenarioAnalyticsInfo());
}

TAnalyticsInfoBuilder& TAnalyticsInfoBuilder::CopyFrom(const TScenarioProto& scenarioProto) {
    *AnalyticsInfo.MutableScenarioAnalyticsInfo() = scenarioProto;
    return *this;
}

TAnalyticsInfoBuilder& TAnalyticsInfoBuilder::Update(const TScenarioProto& scenarioProto) {
    auto timings = std::move(*AnalyticsInfo.MutableScenarioAnalyticsInfo()->MutableScenarioTimings());
    *AnalyticsInfo.MutableScenarioAnalyticsInfo() = scenarioProto;
    if (!timings.GetTimings().empty()) {
        *AnalyticsInfo.MutableScenarioAnalyticsInfo()->MutableScenarioTimings() = std::move(timings);
    }
    return *this;
}

TAnalyticsInfoBuilder& TAnalyticsInfoBuilder::SetVersion(const TString& version) {
    AnalyticsInfo.SetVersion(version);
    return *this;
}

TAnalyticsInfoBuilder& TAnalyticsInfoBuilder::SetSemanticFrame(const TSemanticFrame& frame) {
    AnalyticsInfo.MutableSemanticFrame()->CopyFrom(frame);
    return *this;
}

TAnalyticsInfoBuilder&
TAnalyticsInfoBuilder::SetFrameActions(const google::protobuf::Map<TString, NScenarios::TFrameAction>& actions) {
    *AnalyticsInfo.MutableFrameActions() = actions;
    return *this;
}

TAnalyticsInfoBuilder& TAnalyticsInfoBuilder::SetMatchedSemanticFrames(const TVector<TSemanticFrame>& semanticFrames) {
    for (const auto& semanticFrame : semanticFrames) {
        *AnalyticsInfo.AddMatchedSemanticFrames() = semanticFrame;
    }
    return *this;
}

TAnalyticsInfoBuilder& TAnalyticsInfoBuilder::SetParentRequestId(const TString& requestId) {
    *AnalyticsInfo.MutableParentRequestId() = requestId;
    return *this;
}

TAnalyticsInfoBuilder& TAnalyticsInfoBuilder::SetParentProductScenarioName(const TString& productScenarioName) {
    *AnalyticsInfo.MutableParentProductScenarioName() = productScenarioName;
    return *this;
}

TAnalyticsInfo TAnalyticsInfoBuilder::Build() {
    return std::move(AnalyticsInfo);
}

// TUserInfoBuilder ------------------------------------------------------------
THolder<NScenarios::IUserInfoBuilder> TUserInfoBuilder::CreateScenarioUserInfoBuilder() {
    if (!UserInfo) {
        UserInfo = MakeHolder<TUserInfo>();
    }
    return MakeHolder<NScenarios::TUserInfoBuilder>(*UserInfo->MutableScenarioUserInfo());
}

TUserInfoBuilder& TUserInfoBuilder::CopyFrom(const TScenarioProto& scenarioProto) {
    if (!UserInfo) {
        UserInfo = MakeHolder<TUserInfo>();
    }
    *UserInfo->MutableScenarioUserInfo() = scenarioProto;
    return *this;
}

THolder<TUserInfo> TUserInfoBuilder::Build() && {
    if (UserInfo && UserInfo->HasScenarioUserInfo()) {
        return std::move(UserInfo);
    }
    return nullptr;
}

} // namespace NAlice::NMegamind

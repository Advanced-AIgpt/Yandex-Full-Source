#include "megamind_analytics_info.h"

#include <alice/megamind/protos/analytics/recognized_action_info.pb.h>
#include <alice/megamind/protos/common/device_state.pb.h>
#include <alice/megamind/protos/common/frame.pb.h>
#include <alice/megamind/protos/common/location.pb.h>
#include <alice/megamind/protos/proactivity/proactivity.pb.h>
#include <alice/megamind/protos/scenarios/analytics_info.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>
#include <alice/megamind/protos/scenarios/user_info.pb.h>

#include <google/protobuf/util/message_differencer.h>

namespace NAlice::NMegamind {

namespace {

// TODO(MEGAMIND-1506): delete code after removing all scenarios from megamind
bool IsEmptyScenarioUserInfo(const NScenarios::TUserInfo& userInfo) {
    return google::protobuf::util::MessageDifferencer::Equals(userInfo, NScenarios::TUserInfo::default_instance());
}

// TODO(MEGAMIND-1506): delete code after removing all scenarios from megamind
bool IsEmptyScenarioAnalyticsInfo(const NScenarios::TAnalyticsInfo& analyticsInfo) {
    return google::protobuf::util::MessageDifferencer::Equals(analyticsInfo,
                                                              NScenarios::TAnalyticsInfo::default_instance());
}

TRecognizedActionInfo CreateRecognizedAction(
    TRecognizedActionInfo::EOrigin origin,
    const TString& parentRequestId,
    const TString& actionId,
    const TString& parentProductScenarioName = {}
) {
    TRecognizedActionInfo info;
    info.SetOrigin(origin);
    info.SetParentRequestId(parentRequestId);
    info.SetActionId(actionId);
    if (!parentProductScenarioName.empty()) {
        info.SetParentProductScenarioName(parentProductScenarioName);
    }
    return info;
}

} // namespace

TMegamindAnalyticsInfoBuilder& TMegamindAnalyticsInfoBuilder::SetOriginalUtterance(const TString& originalUtterance) {
    MegamindAnalyticsInfo.SetOriginalUtterance(originalUtterance);
    return *this;
}

TMegamindAnalyticsInfoBuilder& TMegamindAnalyticsInfoBuilder::SetChosenUtterance(const TString& utterance) {
    MegamindAnalyticsInfo.SetChosenUtterance(utterance);
    return *this;
}

TMegamindAnalyticsInfoBuilder& TMegamindAnalyticsInfoBuilder::SetShownUtterance(TString utterance) {
    MegamindAnalyticsInfo.SetShownUtterance(std::move(utterance));
    return *this;
}

TMegamindAnalyticsInfoBuilder& TMegamindAnalyticsInfoBuilder::SetDeviceStateActions(
    const google::protobuf::Map<TString, TDeviceStateAction>& actions) {
    *MegamindAnalyticsInfo.MutableDeviceStateActions() = actions;
    return *this;
}

TMegamindAnalyticsInfoBuilder&
TMegamindAnalyticsInfoBuilder::SetRecognizedScenarioAction(const TString& parentRequestId, const TString& actionId,
                                                           const TString& parentProductScenarioName) {
    *MegamindAnalyticsInfo.MutableRecognizedAction() =
        CreateRecognizedAction(TRecognizedActionInfo::Scenario, parentRequestId, actionId, parentProductScenarioName);
    return *this;
}

TMegamindAnalyticsInfoBuilder&
TMegamindAnalyticsInfoBuilder::SetRecognizedDeviceAction(const TString& actionId) {
    *MegamindAnalyticsInfo.MutableRecognizedAction() = CreateRecognizedAction(TRecognizedActionInfo::DeviceState,
                                                                              /* parentRequestId= */ "",
                                                                              actionId);
    return *this;
}

TMegamindAnalyticsInfoBuilder&
TMegamindAnalyticsInfoBuilder::AddRecognizedSpaceAction(const TString& recognizedFrame,
                                                        const TAnalyticsTrackingModule& atm) {
    auto& info = *MegamindAnalyticsInfo.AddRecognizedActions();
    info.MutableAnalytics()->CopyFrom(atm);
    info.SetMatchedFrame(recognizedFrame);
    info.SetOrigin(TRecognizedActionInfo::ActiveSpaceAction);
    return *this;
}

TMegamindAnalyticsInfoBuilder& TMegamindAnalyticsInfoBuilder::AddUserInfo(const TString& scenarioName,
                                                                          const TUserInfo& userInfo) {
    if (userInfo.HasScenarioUserInfo() && !IsEmptyScenarioUserInfo(userInfo.GetScenarioUserInfo())) {
        (*MegamindAnalyticsInfo.MutableUsersInfo())[scenarioName] = userInfo;
    }
    return *this;
}

TMegamindAnalyticsInfoBuilder& TMegamindAnalyticsInfoBuilder::AddAnalyticsInfo(const TString& scenarioName,
                                                                               const TAnalyticsInfo& analyticsInfo) {
    auto& mutableAnalyticsInfo = (*MegamindAnalyticsInfo.MutableAnalyticsInfo())[scenarioName];
    if (analyticsInfo.HasScenarioAnalyticsInfo() &&
        !IsEmptyScenarioAnalyticsInfo(analyticsInfo.GetScenarioAnalyticsInfo())) {

        *mutableAnalyticsInfo.MutableScenarioAnalyticsInfo() = analyticsInfo.GetScenarioAnalyticsInfo();
        mutableAnalyticsInfo.MutableScenarioAnalyticsInfo()->ClearTunnellerRawResponses();
    }
    if (analyticsInfo.GetVersion()) {
        mutableAnalyticsInfo.SetVersion(analyticsInfo.GetVersion());
    }
    if (analyticsInfo.HasSemanticFrame()) {
        *mutableAnalyticsInfo.MutableSemanticFrame() = analyticsInfo.GetSemanticFrame();
    }
    if (!analyticsInfo.GetFrameActions().empty()) {
        *mutableAnalyticsInfo.MutableFrameActions() = analyticsInfo.GetFrameActions();
    }
    if (const auto& parentRequestId = analyticsInfo.GetParentRequestId()) {
        mutableAnalyticsInfo.SetParentRequestId(parentRequestId);
    }
    if (const auto& parentProductScenarioName = analyticsInfo.GetParentProductScenarioName()) {
        mutableAnalyticsInfo.SetParentProductScenarioName(parentProductScenarioName);
    }
    if (mutableAnalyticsInfo.GetParentProductScenarioName().empty()) {
        mutableAnalyticsInfo.SetParentProductScenarioName(MegamindAnalyticsInfo.GetParentProductScenarioName());
    }
    mutableAnalyticsInfo.MutableMatchedSemanticFrames()->MergeFrom(analyticsInfo.GetMatchedSemanticFrames());
    return *this;
}

TMegamindAnalyticsInfoBuilder& TMegamindAnalyticsInfoBuilder::AddScenarioTimings(const TString& scenarioName,
                                                                                 const TAnalyticsInfo& analyticsInfo) {
    if (!analyticsInfo.HasScenarioAnalyticsInfo()) {
        return *this;
    }

    const auto& timings = analyticsInfo.GetScenarioAnalyticsInfo().GetScenarioTimings();
    if (!timings.GetTimings().empty()) {
        (*MegamindAnalyticsInfo.MutableScenarioTimings())[scenarioName] = timings;
    }

    return *this;
}

TMegamindAnalyticsInfoBuilder& TMegamindAnalyticsInfoBuilder::SetWinnerScenarioName(const TString& scenarioName) {
    MegamindAnalyticsInfo.MutableWinnerScenario()->SetName(scenarioName);
    return *this;
}

TMegamindAnalyticsInfoBuilder& TMegamindAnalyticsInfoBuilder::SetPreClassifyDuration(ui64 duration) {
    MegamindAnalyticsInfo.SetPreClassifyDuration(duration);
    return *this;
}

TMegamindAnalyticsInfoBuilder& TMegamindAnalyticsInfoBuilder::SetPostClassifyDuration(ui64 duration) {
    MegamindAnalyticsInfo.SetPostClassifyDuration(duration);
    return *this;
}

TMegamindAnalyticsInfoBuilder& TMegamindAnalyticsInfoBuilder::AddServiceSourceTimings(const TString& sourceName,
                                                                                      ui64 duration) {
    (*MegamindAnalyticsInfo.MutableServiceSourceTimings())[sourceName] = duration;
    return *this;
}

TMegamindAnalyticsInfoBuilder& TMegamindAnalyticsInfoBuilder::AddTunnellerRawResponse(const TString& scenarioName,
                                                                                      const TString& response) {
    (*MegamindAnalyticsInfo.MutableTunnellerRawResponses())[scenarioName].AddResponses(response);
    return *this;
}

TMegamindAnalyticsInfoBuilder&
TMegamindAnalyticsInfoBuilder::CopyTunnellerRawResponses(const TString& scenarioName,
                                                         const TAnalyticsInfo& analyticsInfo) {
    if (analyticsInfo.HasScenarioAnalyticsInfo()) {
        const auto& tunnellers = analyticsInfo.GetScenarioAnalyticsInfo().GetTunnellerRawResponses();
        if (!tunnellers.empty()) {
            auto& responses = (*MegamindAnalyticsInfo.MutableTunnellerRawResponses())[scenarioName];
            responses.MutableResponses()->MergeFrom(tunnellers);
        }
    }
    return *this;
}

TMegamindAnalyticsInfoBuilder& TMegamindAnalyticsInfoBuilder::SetModifiersInfo(const TModifiersInfo& modifiersInfo) {
    *MegamindAnalyticsInfo.MutableModifiersInfo() = modifiersInfo;
    MegamindAnalyticsInfo.MutableModifiersAnalyticsInfo()->MutableProactivity()->CopyFrom(
        modifiersInfo.GetProactivity());
    return *this;
}

TMegamindAnalyticsInfoBuilder& TMegamindAnalyticsInfoBuilder::SetIoTUserInfo(const TIoTUserInfo& ioTUserInfo) {
    *MegamindAnalyticsInfo.MutableIoTUserInfo() = ioTUserInfo;
    return *this;
}

TMegamindAnalyticsInfoBuilder& TMegamindAnalyticsInfoBuilder::SetParentProductScenarioName(const TString& productScenario) {
    *MegamindAnalyticsInfo.MutableParentProductScenarioName() = productScenario;
    return *this;
}

TMegamindAnalyticsInfoBuilder& TMegamindAnalyticsInfoBuilder::SetUserProfile(const TUserProfile& userProfile) {
    MegamindAnalyticsInfo.MutableUserProfile()->CopyFrom(userProfile);
    return *this;
}

TMegamindAnalyticsInfoBuilder& TMegamindAnalyticsInfoBuilder::SetLocation(const TRequest::TLocation& sourceLocation) {
    auto& location = *MegamindAnalyticsInfo.MutableLocation();
    location.SetLat(sourceLocation.GetLatitude());
    location.SetLon(sourceLocation.GetLongitude());
    location.SetAccuracy(sourceLocation.GetAccuracy());
    location.SetRecency(sourceLocation.GetRecency());
    location.SetSpeed(sourceLocation.GetSpeed());
    return *this;
}

TMegamindAnalyticsInfoBuilder& TMegamindAnalyticsInfoBuilder::SetModifierAnalyticsInfo(const TModifierAnalyticsInfo& modifierAnalyticsInfo) {
    *MegamindAnalyticsInfo.MutableModifierAnalyticsInfo() = modifierAnalyticsInfo;
    return *this;
}

TMegamindAnalyticsInfoBuilder& TMegamindAnalyticsInfoBuilder::SetCombinatorAnalyticsInfo(
    const NCombinators::TCombinatorAnalyticsInfo& combinatorAnalyticsInfo)
{
    *MegamindAnalyticsInfo.MutableCombinatorsAnalyticsInfo() = combinatorAnalyticsInfo;
    return *this;
}

TMegamindAnalyticsInfoBuilder& TMegamindAnalyticsInfoBuilder::SetModifiersAnalyticsInfo(const NModifiers::TAnalyticsInfo& modifiersAnalyticsInfo) {
    *MegamindAnalyticsInfo.MutableModifiersAnalyticsInfo() = modifiersAnalyticsInfo;
    return *this;
}

const TMegamindAnalyticsInfo& TMegamindAnalyticsInfoBuilder::BuildProto() const {
    return MegamindAnalyticsInfo;
}

NJson::TJsonValue TMegamindAnalyticsInfoBuilder::BuildJson() const {
    return JsonFromProto(MegamindAnalyticsInfo);
}

TMegamindAnalyticsInfoBuilder&
TMegamindAnalyticsInfoBuilder::SetPostroll(const NModifiers::NProactivity::TPostroll& postroll) {
    MegamindAnalyticsInfo.MutableModifiersAnalyticsInfo()->MutablePostroll()->CopyFrom(postroll);
    return *this;
}

} // namespace NAlice::NMegamind

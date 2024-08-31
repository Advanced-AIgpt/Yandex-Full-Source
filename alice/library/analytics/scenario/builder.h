#pragma once

#include <alice/library/analytics/interfaces/analytics_info_builder.h>

#include <alice/megamind/protos/scenarios/analytics_info.pb.h>
#include <alice/megamind/protos/scenarios/user_info.pb.h>

namespace NAlice::NScenarios {

class TAnalyticsInfoBuilder final : public IAnalyticsInfoBuilder, TNonCopyable {
public:
    explicit TAnalyticsInfoBuilder(TAnalyticsInfo& analyticsInfo);

    // IAnalyticsInfoBuilder overrides:
    TAnalyticsInfoBuilder& SetIntentName(const TString& intentName) override;
    TAnalyticsInfoBuilder& SetProductScenarioName(const TString& productScenarioName) override;

    TAnalyticsInfoBuilder& AddActionsFromProto(const TAnalyticsInfo& analyticsInfo) override;
    TAnalyticsInfoBuilder& AddActions(const TVector<TAnalyticsInfo::TAction>& actions) override;
    TAnalyticsInfoBuilder& AddAction(const TString& id, const TString& name, const TString& description) override;
    TAnalyticsInfoBuilder& AddAction(const TAnalyticsInfo::TAction& action) override;

    TAnalyticsInfoBuilder& AddObjectsFromProto(const TAnalyticsInfo& analyticsInfo) override;
    TAnalyticsInfoBuilder& AddObject(const TString& id, const TString& name, const TString& description) override;
    TAnalyticsInfoBuilder& AddObject(const TAnalyticsInfo::TObject& object) override;

    TAnalyticsInfoBuilder& AddEventsFromProto(const TAnalyticsInfo& analyticsInfo) override;
    THolder<IRequestSourceBuilder> AddRequestSourceEvent(const TInstant& instant, const TString& source) override;
    IAnalyticsInfoBuilder& AddSelectedSourceEvent(const TInstant& instant, const TString& source) override;

    THolder<ISelectedWebDocumentBuilder> AddSelectedWebDocumentEvent(const TInstant& instant,
                                                                     const TString& searchType) override;

    IAnalyticsInfoBuilder& AddAnalyticsInfoMusicEvent(const TInstant instant,
                                                      const TMusicEventAnswerType answerType,
                                                      const TString& id,
                                                      const TString& uri) override;

    TAnalyticsInfoBuilder& AddAnalyticsInfoMusicMonitoringEvent(const TInstant& instant,
                                                                bool batchOfTracksRequested,
                                                                bool cacheHit) override;

    THolder<IVideoRequestSourceBuilder> AddVideoRequestSourceEvent(const TInstant& instant,
                                                                   const TString& source) override;

    TAnalyticsInfoBuilder& AddTunnellerRawResponse(const TString& response) override;

    IAnalyticsInfoBuilder& SetStageStartTime(const TString& stage, TInstant startTime) override;
    IAnalyticsInfoBuilder& AddSourceResponseDuration(const TString& stage, const TString& source,
                                                     TDuration duration) override;

private:
    TAnalyticsInfo& AnalyticsInfo;
};

class TUserInfoBuilder final : public IUserInfoBuilder, TNonCopyable {
public:
    explicit TUserInfoBuilder(TUserInfo& userInfo);

    // IUserInfoBuilder overrides:
    THolder<IProfileBuilder> AddProfile(const TString& id, const TString& name, const TString& description) override;
    TUserInfoBuilder& AddIotProfile(const NJson::TJsonValue& iotProfile) override;

private:
    TUserInfo& UserInfo;
};

} // namespace NAlice::NScenarios

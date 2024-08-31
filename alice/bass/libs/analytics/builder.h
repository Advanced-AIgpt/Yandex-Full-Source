#pragma once

#include <alice/library/analytics/scenario/builder.h>
#include <alice/megamind/protos/scenarios/analytics_info.pb.h>

#include <library/cpp/scheme/scheme.h>

namespace NAlice::NScenarios {

class TBassAnalyticsInfoBuilder final : public IAnalyticsInfoBuilder {
public:
    TBassAnalyticsInfoBuilder(const TString& intentName);

    TBassAnalyticsInfoBuilder& SetIntentName(const TString& intentName) override;
    TBassAnalyticsInfoBuilder& SetProductScenarioName(const TString& productScenarioName) override;

    TBassAnalyticsInfoBuilder& AddActionsFromProto(const TAnalyticsInfo& analyticsInfo) override;
    TBassAnalyticsInfoBuilder& AddActions(const TVector<TAnalyticsInfo::TAction>& actions) override;
    TBassAnalyticsInfoBuilder& AddAction(const TString& id, const TString& name, const TString& description) override;
    TBassAnalyticsInfoBuilder& AddAction(const TAnalyticsInfo::TAction& action) override;

    TBassAnalyticsInfoBuilder& AddObjectsFromProto(const TAnalyticsInfo& analyticsInfo) override;
    TBassAnalyticsInfoBuilder& AddObject(const TString& id, const TString& name, const TString& description) override;
    TBassAnalyticsInfoBuilder& AddObject(const TAnalyticsInfo::TObject& object) override;

    TBassAnalyticsInfoBuilder& AddEventsFromProto(const TAnalyticsInfo& analyticsInfo) override;
    THolder<IRequestSourceBuilder> AddRequestSourceEvent(const TInstant& instant, const TString& source) override;
    TBassAnalyticsInfoBuilder& AddSelectedSourceEvent(const TInstant& instant, const TString& source) override;

    THolder<ISelectedWebDocumentBuilder> AddSelectedWebDocumentEvent(const TInstant& instant, const TString& searchType) override;

    TBassAnalyticsInfoBuilder& AddAnalyticsInfoMusicEvent(const TInstant instant,
                                                          const TMusicEventAnswerType answerType,
                                                          const TString& id,
                                                          const TString& uri) override;

    TBassAnalyticsInfoBuilder& AddAnalyticsInfoMusicMonitoringEvent(const TInstant& instant,
                                                                    bool batchOfTracksRequested,
                                                                    bool cacheHit) override;

    THolder<IVideoRequestSourceBuilder> AddVideoRequestSourceEvent(const TInstant& instant, const TString& source) override;
    TBassAnalyticsInfoBuilder& AddTunnellerRawResponse(const TString& response) override;

    TString SerializeAsString() const;
    TString SerializeAsBase64() const;

    const TAnalyticsInfo& Build() const {
        return Proto;
    }

    TBassAnalyticsInfoBuilder& SetStageStartTime(const TString& stage, TInstant startTime) override;
    TBassAnalyticsInfoBuilder& AddSourceResponseDuration(const TString& stage, const TString& source,
                                                         TDuration duration) override;

private:
    TAnalyticsInfo Proto;
};

} // namespace NAlice::NScenarios

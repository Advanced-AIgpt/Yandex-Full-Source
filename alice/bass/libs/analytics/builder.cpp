#include "builder.h"

#include <library/cpp/string_utils/base64/base64.h>

namespace NAlice::NScenarios {

// TBassAnalyticsInfoBuilder ---------------------------------------------------
TBassAnalyticsInfoBuilder::TBassAnalyticsInfoBuilder(const TString& intentName) {
    SetIntentName(intentName);
}

TBassAnalyticsInfoBuilder& TBassAnalyticsInfoBuilder::SetIntentName(const TString& intentName) {
    TAnalyticsInfoBuilder{Proto}.SetIntentName(intentName);
    return *this;
}

TBassAnalyticsInfoBuilder& TBassAnalyticsInfoBuilder::SetProductScenarioName(const TString& productScenarioName) {
    TAnalyticsInfoBuilder{Proto}.SetProductScenarioName(productScenarioName);
    return *this;
}

TBassAnalyticsInfoBuilder& TBassAnalyticsInfoBuilder::AddActionsFromProto(const TAnalyticsInfo& analyticsInfo) {
    TAnalyticsInfoBuilder{Proto}.AddActionsFromProto(analyticsInfo);
    return *this;
}

TBassAnalyticsInfoBuilder& TBassAnalyticsInfoBuilder::AddActions(const TVector<TAnalyticsInfo::TAction>& actions) {
    TAnalyticsInfoBuilder{Proto}.AddActions(actions);
    return *this;
}

TBassAnalyticsInfoBuilder& TBassAnalyticsInfoBuilder::AddAction(const TString& id, const TString& name,
                                                                const TString& description) {
    TAnalyticsInfoBuilder{Proto}.AddAction(id, name, description);
    return *this;
}

TBassAnalyticsInfoBuilder& TBassAnalyticsInfoBuilder::AddAction(const TAnalyticsInfo::TAction& action) {
    TAnalyticsInfoBuilder{Proto}.AddAction(action);
    return *this;
}

TBassAnalyticsInfoBuilder& TBassAnalyticsInfoBuilder::AddObjectsFromProto(const TAnalyticsInfo& analyticsInfo) {
    TAnalyticsInfoBuilder{Proto}.AddObjectsFromProto(analyticsInfo);
    return *this;
}

TBassAnalyticsInfoBuilder& TBassAnalyticsInfoBuilder::AddObject(const TString& id, const TString& name,
                                                                const TString& description) {
    TAnalyticsInfoBuilder{Proto}.AddObject(id, name, description);
    return *this;
}

TBassAnalyticsInfoBuilder& TBassAnalyticsInfoBuilder::AddObject(const TAnalyticsInfo::TObject& object) {
    TAnalyticsInfoBuilder{Proto}.AddObject(object);
    return *this;
}

TBassAnalyticsInfoBuilder& TBassAnalyticsInfoBuilder::AddEventsFromProto(const TAnalyticsInfo& analyticsInfo) {
    TAnalyticsInfoBuilder{Proto}.AddEventsFromProto(analyticsInfo);
    return *this;
}

THolder<IAnalyticsInfoBuilder::IRequestSourceBuilder>
TBassAnalyticsInfoBuilder::AddRequestSourceEvent(const TInstant& instant, const TString& source) {
    return TAnalyticsInfoBuilder{Proto}.AddRequestSourceEvent(instant, source);
}

TBassAnalyticsInfoBuilder& TBassAnalyticsInfoBuilder::AddSelectedSourceEvent(const TInstant& instant, const TString& source) {
    TAnalyticsInfoBuilder{Proto}.AddSelectedSourceEvent(instant, source);
    return *this;
}

THolder<IAnalyticsInfoBuilder::ISelectedWebDocumentBuilder>
TBassAnalyticsInfoBuilder::AddSelectedWebDocumentEvent(const TInstant& instant, const TString& searchType) {
    return TAnalyticsInfoBuilder{Proto}.AddSelectedWebDocumentEvent(instant, searchType);
}

TBassAnalyticsInfoBuilder& TBassAnalyticsInfoBuilder::AddAnalyticsInfoMusicEvent(
    const TInstant instant,
    const TMusicEventAnswerType answerType,
    const TString& id,
    const TString& uri)
{
    TAnalyticsInfoBuilder{Proto}.AddAnalyticsInfoMusicEvent(instant, answerType, id, uri);
    return *this;
}

TBassAnalyticsInfoBuilder& TBassAnalyticsInfoBuilder::AddAnalyticsInfoMusicMonitoringEvent(const TInstant& instant,
    bool batchOfTracksRequested, bool cacheHit) {
    TAnalyticsInfoBuilder{Proto}.AddAnalyticsInfoMusicMonitoringEvent(instant, batchOfTracksRequested, cacheHit);
    return *this;
}

THolder<IAnalyticsInfoBuilder::IVideoRequestSourceBuilder>
TBassAnalyticsInfoBuilder::AddVideoRequestSourceEvent(const TInstant& instant, const TString& source) {
    return TAnalyticsInfoBuilder{Proto}.AddVideoRequestSourceEvent(instant, source);
}

TBassAnalyticsInfoBuilder& TBassAnalyticsInfoBuilder::AddTunnellerRawResponse(const TString& response) {
    TAnalyticsInfoBuilder{Proto}.AddTunnellerRawResponse(response);
    return *this;
}

TBassAnalyticsInfoBuilder& TBassAnalyticsInfoBuilder::SetStageStartTime(const TString& stage, TInstant startTime) {
    TAnalyticsInfoBuilder{Proto}.SetStageStartTime(stage, startTime);
    return *this;
}

TBassAnalyticsInfoBuilder&
TBassAnalyticsInfoBuilder::AddSourceResponseDuration(const TString& stage, const TString& source, TDuration duration) {
    TAnalyticsInfoBuilder{Proto}.AddSourceResponseDuration(stage, source, duration);
    return *this;
}

TString TBassAnalyticsInfoBuilder::SerializeAsString() const {
    TString buffer;
    Y_ENSURE(Proto.SerializeToString(&buffer), "Failed to serialize provided Proto to String");

    return buffer;
}

TString TBassAnalyticsInfoBuilder::SerializeAsBase64() const {
    return Base64Encode(SerializeAsString());
}

} // namespace NAlice::NScenarios

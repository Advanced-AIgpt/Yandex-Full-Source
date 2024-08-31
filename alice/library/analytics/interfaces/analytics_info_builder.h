#pragma once

#include <alice/megamind/protos/scenarios/analytics_info.pb.h>

#include <library/cpp/json/json_value.h>

#include <util/datetime/base.h>
#include <util/generic/vector.h>

namespace NAlice::NScenarios {

using TMusicEventAnswerType = TAnalyticsInfo_TEvent_TMusicEvent_EAnswerType;

class IAnalyticsInfoBuilder {
public:
    class IRequestSourceBuilder {
    public:
        virtual ~IRequestSourceBuilder() = default;

        virtual IRequestSourceBuilder& AddCgiParam(const TString& key, const TString& value) = 0;

        virtual IRequestSourceBuilder& AddHeader(const TString& header, const TString& value) = 0;
        virtual IRequestSourceBuilder& SetResponseCode(ui32 code, bool success) = 0;
        virtual IRequestSourceBuilder& SetResponseBody(const TString& body) = 0;

        virtual IAnalyticsInfoBuilder& Build() const = 0;
    };

    class ISelectedWebDocumentBuilder {
    public:
        virtual ~ISelectedWebDocumentBuilder() = default;

        virtual ISelectedWebDocumentBuilder& SetRequestId(const TString& reqId) = 0;
        virtual ISelectedWebDocumentBuilder& SetSearchRequestId(const TString& searchReqId) = 0;
        virtual ISelectedWebDocumentBuilder& SetDocumentUrl(const TString& documentUrl) = 0;
        virtual ISelectedWebDocumentBuilder& SetDocumentPos(ui32 documentPos) = 0;
        virtual ISelectedWebDocumentBuilder& SetCatalogUrl(const TString& catalogUrl) = 0;
        virtual ISelectedWebDocumentBuilder& SetAnswerUrl(const TString& answerUrl) = 0;

        virtual IAnalyticsInfoBuilder& Build() const = 0;
    };

    class IVideoRequestSourceBuilder {
    public:
        virtual ~IVideoRequestSourceBuilder() = default;

        virtual IVideoRequestSourceBuilder& AddCgiParam(const TString& key, const TString& value) = 0;

        virtual IVideoRequestSourceBuilder& AddHeader(const TString& header, const TString& value) = 0;
        virtual IVideoRequestSourceBuilder& SetResponseCode(ui32 code, bool success) = 0;
        virtual IVideoRequestSourceBuilder& SetResponseBody(const TString& body) = 0;
        virtual IVideoRequestSourceBuilder& SetRequestUrl(const TString& url) = 0;
        virtual IVideoRequestSourceBuilder& SetRequestText(const TString& text) = 0;

        virtual IAnalyticsInfoBuilder& Build() const = 0;
    };

    virtual ~IAnalyticsInfoBuilder() = default;

    virtual IAnalyticsInfoBuilder& SetIntentName(const TString& intentName) = 0;
    virtual IAnalyticsInfoBuilder& SetProductScenarioName(const TString& productScenarioName) = 0;

    virtual IAnalyticsInfoBuilder& AddActionsFromProto(const TAnalyticsInfo& analyticsInfo) = 0;
    virtual IAnalyticsInfoBuilder& AddActions(const TVector<TAnalyticsInfo::TAction>& actions) = 0;
    virtual IAnalyticsInfoBuilder& AddAction(const TString& id, const TString& name, const TString& description) = 0;
    virtual IAnalyticsInfoBuilder& AddAction(const TAnalyticsInfo::TAction& action) = 0;

    virtual IAnalyticsInfoBuilder& AddObjectsFromProto(const TAnalyticsInfo& analyticsInfo) = 0;
    virtual IAnalyticsInfoBuilder& AddObject(const TString& id, const TString& name, const TString& description) = 0;
    virtual IAnalyticsInfoBuilder& AddObject(const TAnalyticsInfo::TObject& object) = 0;

    virtual IAnalyticsInfoBuilder& AddEventsFromProto(const TAnalyticsInfo& analyticsInfo) = 0;
    virtual THolder<IRequestSourceBuilder> AddRequestSourceEvent(const TInstant& instant, const TString& source) = 0;

    virtual IAnalyticsInfoBuilder& AddSelectedSourceEvent(const TInstant& instant, const TString& source) = 0;

    virtual THolder<ISelectedWebDocumentBuilder> AddSelectedWebDocumentEvent(const TInstant& instant,
                                                                             const TString& searchType) = 0;

    virtual IAnalyticsInfoBuilder& AddAnalyticsInfoMusicEvent(const TInstant instant,
                                                              const TMusicEventAnswerType answerType,
                                                              const TString& id,
                                                              const TString& uri) = 0;

    virtual IAnalyticsInfoBuilder& AddAnalyticsInfoMusicMonitoringEvent(const TInstant& instant,
                                                                        bool batchOfTracksRequested,
                                                                        bool cacheHit) = 0;

    virtual THolder<IVideoRequestSourceBuilder> AddVideoRequestSourceEvent(const TInstant& instant,
                                                                           const TString& source) = 0;

    virtual IAnalyticsInfoBuilder& AddTunnellerRawResponse(const TString& response) = 0;

    virtual IAnalyticsInfoBuilder& SetStageStartTime(const TString& stage, TInstant startTime) = 0;
    virtual IAnalyticsInfoBuilder& AddSourceResponseDuration(const TString& stage, const TString& source,
                                                             TDuration duration) = 0;
};

class IUserInfoBuilder {
public:
    class IProfileBuilder {
    public:
        virtual ~IProfileBuilder() = default;

        virtual IProfileBuilder& AddParams(const TString& key, const TString& value, const TString& description) = 0;

        virtual IUserInfoBuilder& Build() const = 0;
    };

    virtual ~IUserInfoBuilder() = default;

    virtual THolder<IProfileBuilder> AddProfile(const TString& id, const TString& name,
                                                const TString& description) = 0;
    virtual IUserInfoBuilder& AddIotProfile(const NJson::TJsonValue& iotProfile) = 0;
};

} // namespace NAlice::NScenarios

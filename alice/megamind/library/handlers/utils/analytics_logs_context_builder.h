#pragma once

#include <alice/megamind/library/apphost_request/protos/analytics_logs_context.pb.h>
#include <alice/megamind/library/util/status.h>

#include <library/cpp/http/misc/httpcodes.h>

namespace NAlice::NMegamind {

class IAnalyticsLogContextBuilder {
public:
    virtual IAnalyticsLogContextBuilder& SetHttpCode(HttpCodes httpCode) = 0;
    virtual IAnalyticsLogContextBuilder& SetHideSensitiveData(bool hideSensitiveData) = 0;
    virtual IAnalyticsLogContextBuilder& SetResponse(const TErrorOr<TSpeechKitResponseProto>& response) = 0;
    virtual IAnalyticsLogContextBuilder& SetAnalyticsInfo(const TMegamindAnalyticsInfo& analyticsInfo) = 0;
    virtual IAnalyticsLogContextBuilder& SetQualityStorage(const TQualityStorage& qualityStorage) = 0;
    virtual IAnalyticsLogContextBuilder& SetProactivityLogStorage(const TProactivityLogStorage& proactivity) = 0;

    virtual NMegamindAppHost::TAnalyticsLogsContext BuildProto() = 0;
};

class TAnalyticsLogContextBuilder : public IAnalyticsLogContextBuilder {
public:
    TAnalyticsLogContextBuilder();

    IAnalyticsLogContextBuilder& SetHttpCode(HttpCodes httpCode) override;
    IAnalyticsLogContextBuilder& SetHideSensitiveData(bool hideSensitiveData) override;
    IAnalyticsLogContextBuilder& SetResponse(const TErrorOr<TSpeechKitResponseProto>& response) override;
    IAnalyticsLogContextBuilder& SetAnalyticsInfo(const TMegamindAnalyticsInfo& analyticsInfo) override;
    IAnalyticsLogContextBuilder& SetQualityStorage(const TQualityStorage& qualityStorage) override;
    IAnalyticsLogContextBuilder& SetProactivityLogStorage(const TProactivityLogStorage& proactivity) override;

    NMegamindAppHost::TAnalyticsLogsContext BuildProto() override;
private:
    NMegamindAppHost::TAnalyticsLogsContext Proto;
};

} // namespace NAlice::NMegamind

#include "analytics_logs_context_builder.h"

#include <alice/megamind/protos/analytics/megamind_analytics_info.pb.h>
#include <alice/megamind/protos/proactivity/proactivity.pb.h>
#include <alice/megamind/protos/quality_storage/storage.pb.h>

#include <alice/megamind/library/apphost_request/util.h>

namespace NAlice::NMegamind {

TAnalyticsLogContextBuilder::TAnalyticsLogContextBuilder() {
    SetHideSensitiveData(HTTP_OK);
    SetHideSensitiveData(true);
}

IAnalyticsLogContextBuilder& TAnalyticsLogContextBuilder::SetHttpCode(HttpCodes httpCode) {
    Proto.SetHttpCode(((int)httpCode));
    return *this;
}

IAnalyticsLogContextBuilder& TAnalyticsLogContextBuilder::SetHideSensitiveData(bool hideSensitiveData) {
    Proto.SetHideSensitiveData(hideSensitiveData);
    return *this;
}

IAnalyticsLogContextBuilder&
TAnalyticsLogContextBuilder::SetResponse(const TErrorOr<TSpeechKitResponseProto>& response) {
    if (response.Error()) {
        *Proto.MutableError() = ErrorToProto(*response.Error());
    } else {
        *Proto.MutableSpeechKitResponse() = response.Value();
    }
    return *this;
}

IAnalyticsLogContextBuilder&
TAnalyticsLogContextBuilder::SetAnalyticsInfo(const TMegamindAnalyticsInfo& analyticsInfo) {
    *Proto.MutableAnalyticsInfo() = analyticsInfo;
    return *this;
}

IAnalyticsLogContextBuilder& TAnalyticsLogContextBuilder::SetQualityStorage(const TQualityStorage& qualityStorage) {
    *Proto.MutableQualityStorage() = qualityStorage;
    return *this;
}

IAnalyticsLogContextBuilder&
TAnalyticsLogContextBuilder::SetProactivityLogStorage(const TProactivityLogStorage& proactivity) {
    *Proto.MutableProactivityLogStorage() = proactivity;
    return *this;
}

NMegamindAppHost::TAnalyticsLogsContext TAnalyticsLogContextBuilder::BuildProto() {
    return Proto;
}
} // namespace NAlice::NMegamind

#include "utils.h"

#include "builder.h"

#include <alice/megamind/api/response/constructor.h>

#include <alice/megamind/protos/common/content_properties.pb.h>

#include <alice/library/analytics/common/names.h>
#include <alice/library/json/json.h>
#include <alice/library/proto/protobuf.h>

#include <google/protobuf/util/json_util.h>

#include <library/cpp/protobuf/json/proto2json.h>

#include <util/charset/utf8.h>

#include <utility>

namespace NAlice {

NJson::TJsonValue SpeechKitResponseToJson(const TSpeechKitResponseProto& skResponse) {
    NMegamindApi::TResponseConstructor responseConstructor{};
    responseConstructor.PushSpeechKitProto(skResponse);
    return std::move(responseConstructor).MakeResponse();
}

// TScenarioResponseVisitor ----------------------------------------------------
TScenarioResponseVisitor::TScenarioResponseVisitor(const NMegamind::TMegamindAnalyticsInfoBuilder* analyticsBuilder,
                                                   const TQualityStorage* qualityStorage,
                                                   const NMegamind::TProactivityLogStorage* proactivityLogStorage)
    : AnalyticsInfoBuilder{analyticsBuilder}
    , QualityStorage{qualityStorage}
    , ProactivityLogStorage{proactivityLogStorage} {
}

TSpeechKitResponseProto TScenarioResponseVisitor::operator() (const TDirectiveListResponse& directiveListResponse) const {
    auto response = directiveListResponse.GetProto();
    EnrichResponse(response);
    return response;
}

TSpeechKitResponseProto TScenarioResponseVisitor::operator()(const TScenarioResponse& response) const {
    auto skrResponse = ProcessResponseBuilder(response);
    if (response.GetHttpCode() == HTTP_UNASSIGNED_512) {
        auto& error = *skrResponse.MutableError();
        error.SetCode(HTTP_UNASSIGNED_512);
        if (const auto& reason = response.GetHttpErrorReason(); reason.Defined()) {
            error.SetReason(*reason);
        }
    }
    return skrResponse;
}

void TScenarioResponseVisitor::EnrichResponse(TSpeechKitResponseProto& response) const {
    if (QualityStorage) {
        *response.MutableResponse()->MutableQualityStorage() = *QualityStorage;
    }
    if (AnalyticsInfoBuilder) {
        *response.MutableMegamindAnalyticsInfo() = AnalyticsInfoBuilder->BuildProto();
    }
    if (ProactivityLogStorage) {
        *response.MutableProactivityLogStorage() = *ProactivityLogStorage;
    }
}

TSpeechKitResponseProto TScenarioResponseVisitor::ProcessResponseBuilder(const TResponseBuilder& builder) const {
    auto skResponse = builder.GetSKRProto();
    EnrichResponse(skResponse);
    return skResponse;
}

TSpeechKitResponseProto TScenarioResponseVisitor::ProcessResponseBuilder(const TScenarioResponse& response) const {
    if (const auto* builder = response.BuilderIfExists()) {
        return ProcessResponseBuilder(*builder);
    }
    return TSpeechKitResponseProto::default_instance();
}

bool CheckResponseSensitivity(const TScenarioResponse& response) {
    if (const auto* builder = response.BuilderIfExists()) {
        const auto& contentProperties = builder->GetContentProperties();
        return contentProperties.GetContainsSensitiveDataInRequest() ||
               contentProperties.GetContainsSensitiveDataInResponse();
    }
    return false;
}

} // namespace NAlice

#pragma once

#include "response.h"

#include <alice/megamind/library/analytics/megamind_analytics_info.h>
#include <alice/megamind/library/context/context.h>
#include <alice/megamind/library/util/status.h>
#include <alice/megamind/library/walker/talkien.h>

#include <alice/megamind/protos/proactivity/proactivity.pb.h>
#include <alice/megamind/protos/quality_storage/storage.pb.h>
#include <alice/megamind/protos/speechkit/response.pb.h>

#include <alice/library/logger/logger.h>

#include <library/cpp/json/json_value.h>

#include <util/generic/string.h>

namespace NAlice {

class TResponseBuilder;

NJson::TJsonValue SpeechKitResponseToJson(const TSpeechKitResponseProto& skResponse);

class TScenarioResponseVisitor final {
public:
    explicit TScenarioResponseVisitor(const NMegamind::TMegamindAnalyticsInfoBuilder* analyticsInfoBuilder = nullptr,
                                      const TQualityStorage* qualityStorage = nullptr,
                                      const NMegamind::TProactivityLogStorage* proactivityLogStorage = nullptr);

    TSpeechKitResponseProto operator()(const TDirectiveListResponse& directiveListResponse) const;
    TSpeechKitResponseProto operator()(const TScenarioResponse& response) const;

private:
    [[nodiscard]] TSpeechKitResponseProto ProcessResponseBuilder(const TScenarioResponse& response) const;

    [[nodiscard]] TSpeechKitResponseProto ProcessResponseBuilder(const TResponseBuilder& builder) const;

    void EnrichResponse(TSpeechKitResponseProto& response) const;

    const NMegamind::TMegamindAnalyticsInfoBuilder* AnalyticsInfoBuilder;
    const TQualityStorage* QualityStorage;
    const NMegamind::TProactivityLogStorage* ProactivityLogStorage;
};

bool CheckResponseSensitivity(const TScenarioResponse& response);

} // namespace NAlice

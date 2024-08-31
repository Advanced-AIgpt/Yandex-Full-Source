#pragma once

#include "server_directive_model.h"

#include <alice/megamind/protos/common/atm.pb.h>
#include <alice/megamind/protos/common/frame.pb.h>
#include <alice/megamind/protos/common/frame_request_params.pb.h>
#include <alice/megamind/protos/common/request_params.pb.h>
#include <alice/megamind/protos/scenarios/frame.pb.h>

namespace NAlice::NMegamind {

class TTypedSemanticFrameRequestDirectiveModel : public TServerDirectiveModel {
public:
    TTypedSemanticFrameRequestDirectiveModel(const TTypedSemanticFrame& frame,
                                             const TAnalyticsTrackingModule& analytics,
                                             const TMaybe<TFrameRequestParams>& params,
                                             const TMaybe<TRequestParams>& requestParams,
                                             const TMaybe<TString> utterance = Nothing());

    explicit TTypedSemanticFrameRequestDirectiveModel(const NScenarios::TParsedUtterance& parsedUtterance);
    explicit TTypedSemanticFrameRequestDirectiveModel(const TSemanticFrameRequestData& requestData);

    void Accept(IModelSerializer& serializer) const final;

    [[nodiscard]] const NScenarios::TParsedUtterance& GetParsedUtterance() const;
    [[nodiscard]] const TTypedSemanticFrame& GetFrame() const;
    [[nodiscard]] const TAnalyticsTrackingModule& GetAnalytics() const;
    [[nodiscard]] const TString& GetUtterance() const;
    [[nodiscard]] const TMaybe<TFrameRequestParams> GetFrameRequestParams() const;
    [[nodiscard]] const TMaybe<TRequestParams> GetRequestParams() const;

private:
    NScenarios::TParsedUtterance ParsedUtterance;
};

} // namespace NAlice::NMegamind

#include "typed_semantic_frame_request_directive_model.h"

#include <alice/megamind/library/common/defs.h>

#include <utility>

namespace NAlice::NMegamind {

using NScenarios::TParsedUtterance;

TTypedSemanticFrameRequestDirectiveModel::TTypedSemanticFrameRequestDirectiveModel(
    const TTypedSemanticFrame& frame, const TAnalyticsTrackingModule& analytics,
    const TMaybe<TFrameRequestParams>& params, const TMaybe<TRequestParams>& requestParams,
    const TMaybe<TString> utterance)
    : TServerDirectiveModel(TString{SEMANTIC_FRAME_REQUEST_NAME}, /* ignoreAnswer= */ false) {
    ParsedUtterance.MutableTypedSemanticFrame()->CopyFrom(frame);
    ParsedUtterance.MutableAnalytics()->CopyFrom(analytics);
    if (utterance.Defined()) {
        ParsedUtterance.SetUtterance(*utterance);
    }
    if (params.Defined()) {
        ParsedUtterance.MutableParams()->CopyFrom(*params);
    }
    if (requestParams.Defined()) {
        ParsedUtterance.MutableRequestParams()->CopyFrom(*requestParams);
    }
}

TTypedSemanticFrameRequestDirectiveModel::TTypedSemanticFrameRequestDirectiveModel(
    const TParsedUtterance& parsedUtterance)
    : TServerDirectiveModel(TString{SEMANTIC_FRAME_REQUEST_NAME}, /* ignoreAnswer= */ false)
    , ParsedUtterance(parsedUtterance) {
}

TTypedSemanticFrameRequestDirectiveModel::TTypedSemanticFrameRequestDirectiveModel(
    const TSemanticFrameRequestData& requestData)
    : TTypedSemanticFrameRequestDirectiveModel(requestData.GetTypedSemanticFrame(), requestData.GetAnalytics(),
                                               requestData.HasParams()
                                                   ? TMaybe<TFrameRequestParams>(requestData.GetParams())
                                                   : Nothing(),
                                               requestData.HasRequestParams()
                                                   ? TMaybe<TRequestParams>(requestData.GetRequestParams())
                                                   : Nothing()
    ) {
}

void TTypedSemanticFrameRequestDirectiveModel::Accept(IModelSerializer& serializer) const {
    serializer.Visit(*this);
}

const TParsedUtterance& TTypedSemanticFrameRequestDirectiveModel::GetParsedUtterance() const {
    return ParsedUtterance;
}

const TTypedSemanticFrame& TTypedSemanticFrameRequestDirectiveModel::GetFrame() const {
    return ParsedUtterance.GetTypedSemanticFrame();
}

const TAnalyticsTrackingModule& TTypedSemanticFrameRequestDirectiveModel::GetAnalytics() const {
    return ParsedUtterance.GetAnalytics();
}

const TString& TTypedSemanticFrameRequestDirectiveModel::GetUtterance() const {
    return ParsedUtterance.GetUtterance();
}

const TMaybe<TFrameRequestParams> TTypedSemanticFrameRequestDirectiveModel::GetFrameRequestParams() const {
    if (ParsedUtterance.HasParams()) {
        return ParsedUtterance.GetParams();
    }
    return Nothing();
}

const TMaybe<TRequestParams> TTypedSemanticFrameRequestDirectiveModel::GetRequestParams() const {
    if (ParsedUtterance.HasRequestParams()) {
        return ParsedUtterance.GetRequestParams();
    }
    return Nothing();
}

} // namespace NAlice::NMegamind

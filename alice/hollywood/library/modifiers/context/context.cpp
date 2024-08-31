#include "context.h"

namespace NAlice::NHollywood::NModifiers {

TModifierContext::TModifierContext(TRTLogger& logger, const TModifierFeatures& features,
                                   const TModifierBaseRequest& baseRequest, IRng& rng, NMetrics::ISensors& sensors)
    : Logger_{logger}
    , Features{features}
    , BaseRequest{baseRequest}
    , ExpFlags_{ExpFlagsFromProto(baseRequest.GetExperiments())}
    , Rng_{rng}
    , Sensors_{sensors}
{
}

TRTLogger& TModifierContext::Logger() {
    return Logger_;
}

const TModifierBaseRequest& TModifierContext::GetBaseRequest() const {
    return BaseRequest;
}

const TModifierFeatures& TModifierContext::GetFeatures() const {
    return Features;
}

bool TModifierContext::HasExpFlag(TStringBuf name) const {
    return ExpFlags_.FindPtr(name);
}

const TExpFlags& TModifierContext::ExpFlags() const {
    return ExpFlags_;
}

IRng& TModifierContext::Rng() {
    return Rng_;
}

NMetrics::ISensors& TModifierContext::Sensors() {
    return Sensors_;
}

} // namespace NAlice::NHollywood::NModifiers

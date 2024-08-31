#include "nlu_features.h"

#include <alice/hollywood/library/request/request.h>

namespace NAlice::NHollywood {

    TMaybe<float> GetNluFeatureValue(const TScenarioBaseRequestWrapper& baseRequest, NNluFeatures::ENluFeature feature) {
        for (const auto& nluFeature : baseRequest.BaseRequestProto().GetNluFeatures()) {
            if (nluFeature.GetFeature() == feature) {
                return nluFeature.GetValue();
            }
        }
        return Nothing();
    }

} // namespace NAlice::NHollywood

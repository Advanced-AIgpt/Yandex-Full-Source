#include "nlu_features.h"

#include <alice/hollywood/library/framework/core/request_helper.h>

#include <alice/megamind/protos/scenarios/request.pb.h>

namespace NAlice::NHollywoodFw {

TMaybe<float> GetNluFeatureValue(const TRequest& request, NNluFeatures::ENluFeature feature) {
    const auto& proto = NPrivate::TRequestHelper(request).GetBaseRequestProto();
    for (const auto& nluFeature : proto.GetNluFeatures()) {
        if (nluFeature.GetFeature() == feature) {
            return nluFeature.GetValue();
        }
    }
    return Nothing();
}

} // namespace NAlice::NHollywoodFw

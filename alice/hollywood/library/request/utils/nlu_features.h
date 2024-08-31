#include <alice/hollywood/library/request/fwd.h>
#include <alice/protos/api/nlu/generated/features.pb.h>

#include <util/generic/maybe.h>

namespace NAlice::NHollywood {
    // intended to be in TScenarioBaseRequestWrapper, but due to NNluFeatures::ENluFeature dependency was moved out
    TMaybe<float> GetNluFeatureValue(const TScenarioBaseRequestWrapper& baseRequest, NNluFeatures::ENluFeature feature);

} // namespace NAlice::NHollywood

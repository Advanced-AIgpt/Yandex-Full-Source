#pragma once

#include <alice/hollywood/library/framework/core/request.h>

#include <alice/protos/api/nlu/generated/features.pb.h>

#include <util/generic/maybe.h>

namespace NAlice::NHollywoodFw {

//
// Forward declarations
//
class TRequest;

extern TMaybe<float> GetNluFeatureValue(const TRequest& request, NNluFeatures::ENluFeature feature);

} // namespace NAlice::NHollywoodFw

#pragma once

#include <alice/hollywood/library/framework/framework.h>

#include <alice/protos/data/scenario/order/order.pb.h>

namespace NAlice::NHollywoodFw::NOrder {

NAlice::NOrder::TProviderOrderResponse ParseLavkaResponse(const NJson::TJsonValue& jsonResponse, const TRunRequest& request);

}

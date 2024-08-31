#pragma once

#include <apphost/api/service/cpp/service.h>

namespace NAlice::NCuttlefish {

void GolovanMetricsService(const NNeh::IRequestRef& raw);
void SolomonMetricsService(const NNeh::IRequestRef& raw);

} // namespace NAlice::NCuttlefish

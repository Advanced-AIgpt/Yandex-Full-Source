#pragma once

#include <alice/matrix/analytics/enum_value_ordering/enum_value_ordering.h>

#include <alice/matrix/library/logging/events/events.ev.pb.h>

namespace NMatrix::NAnalytics {

using TTechnicalPushValidationResult = NEvClass::TMatrixNotificatorAnalyticsTechnicalPushValidationResult::EResult;

template <>
const NProtoBuf::EnumDescriptor* GetProtoEnumDescriptorForAnalyticsValueOrdering<TTechnicalPushValidationResult>();

} // namespace NMatrix::NAnalytics

#include "enum_value_ordering.h"

namespace NMatrix::NAnalytics {

template <>
const NProtoBuf::EnumDescriptor* GetProtoEnumDescriptorForAnalyticsValueOrdering<TTechnicalPushValidationResult>() {
    return NEvClass::TMatrixNotificatorAnalyticsTechnicalPushValidationResult::EResult_descriptor();
}

} // namespace NMatrix::NAnalytics

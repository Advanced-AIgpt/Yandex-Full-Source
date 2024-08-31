#pragma once

#include <alice/matrix/analytics/protos/enum_value_priority_extension.pb.h>

#include <google/protobuf/descriptor.h>

namespace NMatrix::NAnalytics {

template <typename TEnum>
const NProtoBuf::EnumDescriptor* GetProtoEnumDescriptorForAnalyticsValueOrdering();

namespace NPrivate {

void ValidateEnumValueOrderingExtension(const NProtoBuf::EnumDescriptor* descriptor);

template <typename TEnum>
static bool CompareEnumValuesByAnalyticsPriority(TEnum first, TEnum second) {
    const NProtoBuf::EnumDescriptor* descriptor = GetProtoEnumDescriptorForAnalyticsValueOrdering<TEnum>();
    auto priorityFirst = descriptor->FindValueByNumber(first)->options().GetExtension(::NMatrix::NAnalytics::enum_value_priority);
    auto prioritySecond = descriptor->FindValueByNumber(second)->options().GetExtension(::NMatrix::NAnalytics::enum_value_priority);
    return priorityFirst < prioritySecond;
}

} // namespace NPrivate

template <typename TEnum>
TEnum MaxEnumValueByAnalyticsPriority(TEnum first, TEnum second) {
    if (NPrivate::CompareEnumValuesByAnalyticsPriority(first, second)) {
        return second;
    } else {
        return first;
    }
}

template <typename TEnum>
TEnum MinEnumValueByAnalyticsPriority(TEnum first, TEnum second) {
    if (NPrivate::CompareEnumValuesByAnalyticsPriority(first, second)) {
        return first;
    } else {
        return second;
    }
}

} // namespace NMatrix::NAnalytics

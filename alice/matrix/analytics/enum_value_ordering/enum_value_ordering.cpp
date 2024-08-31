#include "enum_value_ordering.h"

#include <util/generic/hash_set.h>
#include <util/generic/yexception.h>

namespace NMatrix::NAnalytics::NPrivate {

void ValidateEnumValueOrderingExtension(const NProtoBuf::EnumDescriptor* descriptor) {
    using TOrderPriority = decltype(descriptor->value(0)->number());
    THashSet<TOrderPriority> usedPriorities;

    auto value_count = descriptor->value_count();
    for (decltype(value_count) i = 0; i < value_count; ++i) {
        auto value = descriptor->value(i);
        Y_ENSURE(
            value->options().HasExtension(NMatrix::NAnalytics::enum_value_priority),
            "Missing extension option \"enum_value_priority\" in enum " << descriptor->full_name() << ", value " << value->name()
        );

        TOrderPriority priority = static_cast<TOrderPriority>(value->options().GetExtension(NMatrix::NAnalytics::enum_value_priority));
        Y_ENSURE(
            priority >= 0 && priority < value_count,
            "Invalid extension option \"enum_value_priority\" = " << priority << " is out of range [0, value_count) in enum " << descriptor->full_name() << ", value " << value->name()
        );
        
        Y_ENSURE(
            usedPriorities.insert(priority).second,
            "Duplicate extension option \"enum_value_priority\" = " << priority << " in enum " << descriptor->full_name() << ", value " << value->name()
        );
    }
}

} // namespace NMatrix::NAnalytics::NPrivate

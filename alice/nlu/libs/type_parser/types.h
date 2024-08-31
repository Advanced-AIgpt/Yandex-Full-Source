#pragma once
#include <util/generic/string.h>

namespace NAlice {
    enum class EEntityType {
        TEXT,
        DIGITAL,            // "123", "0", "0043", "1 2 3 456 78 9", etc...
        TIME,               // "15 30", "полвторого", "без 5 минут 7", etc...
        UNKNOWN
    };

    TString EntityTypeToString(const EEntityType type);
} // namespace NAlice

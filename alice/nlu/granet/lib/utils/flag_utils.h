#pragma once

#include <util/generic/flags.h>
#include <util/generic/serialized_enum.h>
#include <util/stream/format.h>
#include <util/string/builder.h>

namespace NGranet {

// Flag utils

#define FLAG8(n) (static_cast<ui8>(1UL << static_cast<unsigned>(n)))
#define FLAG16(n) (static_cast<ui16>(1UL << static_cast<unsigned>(n)))
#define FLAG32(n) (static_cast<ui32>(1UL << static_cast<unsigned>(n)))
#define FLAG64(n) (static_cast<ui64>(1UL << static_cast<unsigned>(n)))

// ~~~~ Utils for flags stored in unsigned number ~~~~

template <class T, typename = std::enable_if_t<std::is_unsigned<T>::value>>
inline bool HasFlag(T set, T flag) {
    return (set & flag) == flag;
}

template <class T, typename = std::enable_if_t<std::is_unsigned<T>::value>>
inline bool HasFlags(T set, T flags) {
    return (set & flags) == flags;
}

template <class T, typename = std::enable_if_t<std::is_unsigned<T>::value>>
inline bool HasAnyFlags(T set, T flags) {
    return set & flags;
}

template <class T, typename = std::enable_if_t<std::is_unsigned<T>::value>>
inline void SetFlags(T* set, T flags, bool value = true) {
    Y_ASSERT(set);
    if (value) {
        *set |= flags;
    } else {
        *set &= ~flags;
    }
}

template <class T, typename = std::enable_if_t<std::is_unsigned<T>::value>>
inline T FlagsIf(T flags, bool value = true) {
    return value ? std::move(flags) : T{};
}

// ~~~~ Utils for TFlags ~~~~

template <class TEnum>
inline bool HasIntersection(const TFlags<TEnum>& flags1, const TFlags<TEnum>& flags2) {
    return flags1 & flags2;
}

template <class TEnum>
inline void SetFlags(TFlags<TEnum>* set, const TFlags<TEnum>& flags, bool value = true) {
    Y_ASSERT(set);
    if (value) {
        *set |= flags;
    } else {
        set->RemoveFlags(flags);
    }
}

template <class TEnum>
inline void SetFlags(TFlags<TEnum>* set, TEnum flag, bool value = true) {
    Y_ASSERT(set);
    if (value) {
        *set |= flag;
    } else {
        set->RemoveFlags(flag);
    }
}

template <class TEnum>
inline TFlags<TEnum> FlagsIf(const TFlags<TEnum>& flags, bool value = true) {
    return value ? flags : TFlags<TEnum>{};
}

template <class TEnum>
inline TEnum FlagsIf(TEnum flags, bool value = true) {
    return value ? flags : TEnum{};
}

template <class TEnum>
TString FormatFlags(TFlags<TEnum> flags) {
    if (!flags) {
        return "0";
    }
    TStringBuilder out;
    for (const TEnum value : GetEnumAllValues<TEnum>()) {
        if (!flags.HasFlags(value)) {
            continue;
        }
        if (!out.empty()) {
            out << '|';
        }
        out << value;
        flags.RemoveFlags(value);
    }
    if (flags) {
        if (!out.empty()) {
            out << '|';
        }
        out << Bin(flags.ToBaseType(), HF_ADDX);
    }
    return out;
}

// ~~~~ TFlagsDiff ~~~~

template <class Enum>
struct TFlagsDiff {
    TFlags<Enum> Negative;
    TFlags<Enum> Positive;
};

template <class Enum>
inline TFlagsDiff<Enum> operator-(const TFlagsDiff<Enum>& diff) {
    return {diff.Positive, diff.Negative};
}

template <class Enum>
inline TFlags<Enum> operator+(const TFlags<Enum>& flags, const TFlagsDiff<Enum>& diff) {
    Y_ASSERT(!HasIntersection(diff.Negative, diff.Positive));
    return (flags & ~diff.Negative) | diff.Positive;
}

template <class Enum>
inline TFlags<Enum> operator+(const TFlagsDiff<Enum>& diff, const TFlags<Enum>& flags) {
    return flags + diff;
}

template <class Enum>
inline TFlags<Enum>& operator+=(TFlags<Enum>& flags, const TFlagsDiff<Enum>& diff) {
    return flags = flags + diff;
}

} // namespace NGranet

#pragma once
#include <util/generic/yexception.h>
#include <util/generic/strbuf.h>
#include <util/string/cast.h>


// Split string and convert its' parts into particular types
// Use `nullptr` to skip a part
template <typename T, typename ...Ts>
inline void SplitAndCast(TStringBuf str, char delim, T&& value, Ts&& ...others) {
    Y_ENSURE(TrySplitAndCast(str, delim, std::forward<T>(value), std::forward<Ts>(others)...));
}

template <typename T, typename ...Ts>
bool TrySplitAndCast(TStringBuf str, char delim, T&& value, Ts&& ...others)
{
    TStringBuf left, right;
    if constexpr (sizeof...(others) > 0) {
        if (!str.TrySplit(delim, left, right))
            return false;
        if (!TrySplitAndCast(right, delim, std::forward<Ts>(others)...))
            return false;
    } else {
        left = str;
    }

    if constexpr (!std::is_null_pointer_v<T>)
        return TryFromString(left, value);
    return true;
}

inline void WriteWithReplacing(IOutputStream& out, TStringBuf str, char original, char replacement) {
    for (char c : str)
        out.Write(c == original ? replacement : c);
}

template <typename OutType>
inline void WriteWithSkipping(OutType& out, TStringBuf str, char beSkipped) {
    for (char c : str) {
        if (c != beSkipped)
            out << c;
    }
}


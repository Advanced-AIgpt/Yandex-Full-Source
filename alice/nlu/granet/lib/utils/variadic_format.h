#pragma once

#include <util/stream/str.h>
#include <util/string/cast.h>

namespace NGranet {

namespace NPrivate {

    inline void DoVariadicFormat(IOutputStream& out, int, TStringBuf format) {
        out << format;
    }

    template <class Head, class... Args>
    inline void DoVariadicFormat(IOutputStream& out, int shift, TStringBuf format,
        const Head& head, const Args&... args)
    {
        const TString tag = TString::Join('{', ToString(shift), '}');
        while (!format.empty()) {
            const TStringBuf before = format.NextTokAt(format.find(tag));
            DoVariadicFormat(out, shift + 1, before, args...);
            if (!format.empty()) {
                format.Skip(tag.length());
                out << head;
            }
        }
    }

} // namespace NPrivate

template <typename... Args>
TString VariadicFormat(TStringBuf format, const Args&... args) {
    TString str;
    TStringOutput out(str);
    NPrivate::DoVariadicFormat(out, 0, format, args...);
    return str;
}

} // namespace NGranet

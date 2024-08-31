#pragma once
#include <util/generic/guid.h>
#include <util/string/strip.h>
#include <library/cpp/json/json_reader.h>


TString GuidToUuidString(const TGUID& g);

namespace NPrivate {

    template <char Src, char Dst, char... MorePairs>
    struct TReplaceChars {

        static_assert(sizeof...(MorePairs) % 2 == 0);

        inline static char Replace(char c) {
            if (c == Src)
                return Dst;
            if constexpr (sizeof...(MorePairs) > 0)
                return TReplaceChars<MorePairs...>::Replace(c);
            return c;
        }
    };

}  // namespace NPrivate

template <char ...ReplacePairs>
void ReplaceChars(TString& src) {
    for (char& c : src)
        c = NPrivate::TReplaceChars<ReplacePairs...>::Replace(c);
}


inline NJson::TJsonValue ReadJson(const TStringBuf raw) {
    NJson::TJsonValue json;
    NJson::ReadJsonTree(raw, &json, /* throwOnError = */ true);
    return json;
}

struct TJsonAsPretty {
    const NJson::TJsonValue& Ref;
};

struct TJsonAsDense {
    const NJson::TJsonValue& Ref;
};

TString ExtractSessionIdFromCookie(TStringBuf);

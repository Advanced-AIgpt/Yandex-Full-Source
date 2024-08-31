#pragma once
#include "node.h"
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/bitmap.h>
#include <util/generic/hash.h>
#include <util/str_stl.h>

namespace NVoice {

// Set of fields that identifies one of available profiles
struct TProfileKey {
    TString SpeechkitVersion;
    TString AppId;
    TString AuthToken;

    void Clear() {
        SpeechkitVersion.clear();
        AppId.clear();
        AuthToken.clear();
    }

    inline bool operator==(const TProfileKey& x) const {
        return SpeechkitVersion == x.SpeechkitVersion
            && AppId == x.AppId
            && AuthToken == x.AuthToken;
    }
};

// Bitmasks of required and optional fields
struct TProfileFieldsMask {
    TDynBitMap RequiredFields;
    TDynBitMap AllowedFields;
};

} // namespace NVoice

template <>
struct THash<NVoice::TProfileKey> {
    inline size_t operator()(const NVoice::TProfileKey& x) const {
        return THash<std::tuple<TString, TString, TString>>()(std::make_tuple(
            x.SpeechkitVersion, x.AppId, x.AuthToken
        ));
    }
};


namespace NVoice {

class TParser {
public:
    using TProfilesMap = THashMap<TProfileKey, TProfileFieldsMask>;

public:
    TParser(TNode&& rootNode, TProfilesMap&& profiles);

    bool ParseJson(TStringBuf rawJson);

    // ------------------------------------------------------------------------
    // Methods for debug & tests
    // Results from last parsed JSON, stay valid till next `ParseJson`
    const TProfileKey& GetParsedProfileKey() const {
        return ParsedKey;
    }
    const TDynBitMap& GetParsedFieldsBitmask() const {
        return ParsedFields;
    }
    const TNode& GetRootNode() const {
        return RootNode;
    }
    const TNode* GetNodeByPath(TStringBuf path) const {
        return GetByPath(RootNode, path);
    }
    const TProfilesMap& GetProfilesMap() const {
        return Profiles;
    }
    // ------------------------------------------------------------------------

    template <typename T>
    inline bool operator()(const TNode& node, T value) {
        if constexpr (std::is_same_v<TStringBuf, T>) {
            if (node.Idx == SpeechkitVersionNodeIdx) {
                ParsedKey.SpeechkitVersion = value;
            } else if (node.Idx == AppIdNodeIdx) {
                ParsedKey.AppId = value;
            } else if (node.Idx == AuthTokenNodeIdx) {
                ParsedKey.AuthToken = value;
            }
        }
        ParsedFields.Set(node.Idx);
        return true;
    }

private:
    const TNode RootNode;
    const TProfilesMap Profiles;
    const uint16_t SpeechkitVersionNodeIdx;
    const uint16_t AppIdNodeIdx;
    const uint16_t AuthTokenNodeIdx;

    TProfileKey ParsedKey;
    TDynBitMap ParsedFields;
};

} // namespace NVoice

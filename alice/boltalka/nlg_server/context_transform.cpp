#include "context_transform.h"

#include <util/charset/wide.h>
#include <util/stream/file.h>

namespace NNlgServer {

namespace {

static bool IsPythonPunct(wchar16 ch) {
    static const TUtf16String PUNCT = u"!\"#%&'()*,-./:;?@[\\]_`{}~";
    return IsPunct(ch) || PUNCT.Contains(ch);
}

}

TSeparatePunctuation::TSeparatePunctuation(bool lowerCase, TWtringBuf eosString)
    : LowerCase(lowerCase)
    , EosString(eosString)
{
}

TUtf16String TSeparatePunctuation::Transform(TWtringBuf context) const {
    static const wchar16 MAC_APOSTROPHE = u'’';
    TUtf16String result;
    for (ui64 i = 0; i < context.size(); ++i) {
        wchar16 ch = context[i];
        if (LowerCase) {
            ch = ToLower(ch);
        }
        if (ch == MAC_APOSTROPHE) {
            ch = '\'';
        }
        if (IsPythonPunct(ch)) {
            result += ' ';
        }
        if (ch == '\n') {
            result += EosString;
        } else {
            result += ch;
        }
        if (IsPythonPunct(ch)) {
            result += ' ';
        }
    }
    result += EosString;
    TUtf16String normalizedResult;
    for (ui64 i = 0; i < result.size(); ++i) {
        wchar16 ch = result[i];
        if (ch == ' ' && (i == result.size() - 1 || normalizedResult.empty() || normalizedResult.back() == ' ')) {
            continue;
        }
        normalizedResult += ch;
    }
    return normalizedResult;
}

TLimitNumTokens::TLimitNumTokens(ui64 maxNumTokens)
    : MaxNumTokens(maxNumTokens)
{
}

TUtf16String TLimitNumTokens::Transform(TWtringBuf context) const {
    ui64 spaceCount = 0;
    for (ui64 i = context.size(); i-- > 0; ) {
        if (context[i] == ' ') {
            if (++spaceCount == MaxNumTokens) {
                return ToWtring(context.substr(i + 1));
            }
        }
    }
    return ToWtring(context);
}

TLimitContextLength::TLimitContextLength(ui64 maxContextLength)
    : MaxContextLength(maxContextLength)
{
}

TUtf16String TLimitContextLength::Transform(TWtringBuf context) const {
    ui64 newLineCount = 0;
    for (ui64 i = context.size(); i-- > 0; ) {
        if (context[i] == '\n') {
            if (++newLineCount == MaxContextLength) {
                return ToWtring(context.substr(i + 1));
            }
        }
    }
    return ToWtring(context);
}

TUtf16String TAddCartman::Transform(TWtringBuf context) const {
    return ToWtring(context) + u" _Cartman_";
}

TUtf16String TAddKyleAndCartman::Transform(TWtringBuf context) const {
    TVector<TWtringBuf> replies;
    for (TWtringBuf tok; context.NextTok(u"_EOS_", tok); ) {
        replies.push_back(tok);
    }
    TUtf16String result;
    ui64 parity = replies.size() % 2;
    for (ui64 i = 0; i < replies.size(); ++i) {
        if (i) {
            result += ' ';
        }
        result += UTF8ToWide(i % 2 == parity ? "_Cartman_ " : "_Kyle_ ") + replies[i] + u"_EOS_";
    }
    result += u" _Cartman_";
    return result;
}

TTranslateWithDict::TTranslateWithDict(const TFsPath &path) {
    TSeparatePunctuation sp;
    TFileInput in(path);
    for(TUtf16String line; in.ReadLine(line); ) {
        size_t tabPos = line.find('\t');
        TUtf16String key = line.substr(0, tabPos);

        TUtf16String value = sp.Transform(line.substr(tabPos + 1));
        Y_VERIFY(value.EndsWith(u" _EOS_"));
        value = value.substr(0, value.size() - strlen(" _EOS_"));

        Dict[key] = value;
    }
}

TUtf16String TTranslateWithDict::Transform(TWtringBuf context) const {
    TUtf16String result;
    for (TWtringBuf tok; context.NextTok(' ', tok); ) {
        if (tok.empty()) {
            continue;
        }
        if (!result.empty()) {
            result += ' ';
        }
        auto it = Dict.find(ToWtring(tok));
        if (it != Dict.end()) {
            result += it->second;
        } else {
            result += tok;
        }
    }
    return result;
}

/*TReplaceUnknownTokens::TReplaceUnknownTokens(TTokenDictPtr dict)
    : Dict(dict)
{
}

TUtf16String TReplaceUnknownTokens::Transform(TWtringBuf context) const {
    TUtf16String result;
    for (TWtringBuf tok; context.NextTok(' ', tok); ) {
        if (tok.empty()) {
            continue;
        }
        if (!result.empty()) {
            result += ' ';
        }
        if (Dict->HasToken(tok)) {
            result += tok;
        } else {
            result += u"_UNK_";
        }
    }
    return result;
}*/

TCompoundTransform::TCompoundTransform(TVector<IContextTransformPtr> transforms)
    : Transforms(transforms)
{
}

TUtf16String TCompoundTransform::Transform(TWtringBuf context) const {
    TUtf16String result = ToWtring(context);
    for (const auto &transform : Transforms) {
        result = transform->Transform(result);
    }
    return result;
}

}

#include "utterance_transform.h"

#include <dict/dictutil/dictutil.h>

#include <util/charset/utf8.h>
#include <util/generic/reserve.h>
#include <util/string/subst.h>

namespace NNlgTextUtils {

namespace {

static const TString PYTHON_PUNCTUATION = "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~";

static bool IsPythonPunct(char ch) {
    return PYTHON_PUNCTUATION.Contains(ch);
}

}

TLowerCase::TLowerCase(ELanguage lang)
    : Lang(lang)
{
}

TString TLowerCase::Transform(TStringBuf utterance) const {
    return WideToUTF8(ToLower(Lang, UTF8ToWide(utterance)));
}

TString TSeparatePunctuation::Transform(TStringBuf utterance) const {
    TVector<char> result;
    for (size_t i = 0; i < utterance.size(); ++i) {
        char ch = utterance[i];
        bool isPunct = IsPythonPunct(ch);
        if (isPunct) {
            result.push_back(' ');
        }
        result.push_back(ch);
        if (isPunct) {
            result.push_back(' ');
        }
    }
    size_t writePos = 0;
    for (size_t i = 0; i < result.size(); ++i) {
        if (result[i] != ' ' || i && result[i - 1] != ' ') {
            result[writePos++] = result[i];
        }
    }
    if (writePos && result[writePos - 1] == ' ') {
        --writePos;
    }
    return {result.data(), writePos};
}

TString TRemovePunctuation::Transform(TStringBuf utterance) const {
    TString result(Reserve(utterance.size()));
    for (const auto& symbol : utterance) {
        if (!IsPythonPunct(symbol)) {
            result += symbol;
        }
    }
    return result;
}

TLimitNumTokens::TLimitNumTokens(size_t maxNumTokens)
    : MaxNumTokens(maxNumTokens)
{
}

TString TLimitNumTokens::Transform(TStringBuf utterance) const {
    size_t spaceCount = 0;
    for (size_t i = 0; i < utterance.size(); ++i) {
        if (utterance[i] == ' ') {
            if (++spaceCount == MaxNumTokens) {
                return TString{utterance.SubStr(0, i)};
            }
        }
    }
    return TString{utterance};
}

TReplacePunct4Insight::TReplacePunct4Insight() {
    for (size_t i = 0; i < PYTHON_PUNCTUATION.size(); ++i) {
        char from = PYTHON_PUNCTUATION[i];
        TString to(1, 'A' + i % 26);
        if (i >= 26) {
            to += '0';
        }
        Dict[from] = to;
    }
}

TString TReplacePunct4Insight::Transform(TStringBuf utterance) const {
    TString result;
    for (size_t i = 0; i < utterance.size(); ++i) {
        char ch = utterance[i];
        auto it = Dict.find(ch);
        if (it != Dict.end()) {
            result += it->second;
        } else {
            result += ch;
        }
    }
    return result;
}

const TVector<TString> TMapYo::From = { "ё", "ë" };
const TString TMapYo::To = "е";

TString TMapYo::Transform(TStringBuf utterance) const {
    TString result = TString{utterance};
    for (const auto& from : From) {
        SubstGlobal(result, from, To);
    }
    return result;
}

TCompoundUtteranceTransform::TCompoundUtteranceTransform(TVector<IUtteranceTransformPtr> transforms)
    : Transforms(transforms)
{
}

TString TCompoundUtteranceTransform::Transform(TStringBuf utterance) const {
    TString result = TString{utterance};
    for (const auto& transform : Transforms) {
        result = transform->Transform(result);
    }
    return result;
}

const size_t TNlgSearchUtteranceTransform::MaxNumTokens = 100;

TNlgSearchUtteranceTransform::TNlgSearchUtteranceTransform(ELanguage lang)
    : LowerCase(lang)
    , LimitNumTokens(MaxNumTokens)
{
}

TString TNlgSearchUtteranceTransform::Transform(TStringBuf utterance) const {
    TString result = TString{utterance};
    result = LowerCase.Transform(result);
    result = SeparatePunct.Transform(result);
    result = MapYo.Transform(result);
    result = LimitNumTokens.Transform(result);
    return result;
}

}

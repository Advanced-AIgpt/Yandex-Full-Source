#pragma once

#include <library/cpp/langs/langs.h>

#include <util/generic/string.h>
#include <util/generic/ptr.h>
#include <util/generic/hash.h>
#include <util/generic/vector.h>

namespace NNlgTextUtils {

class IUtteranceTransform : public TThrRefBase {
public:
    virtual ~IUtteranceTransform() = default;
    virtual TString Transform(TStringBuf utterance) const = 0;
};
using IUtteranceTransformPtr = TIntrusivePtr<IUtteranceTransform>;

class TLowerCase : public IUtteranceTransform {
public:
    explicit TLowerCase(ELanguage lang);
    TString Transform(TStringBuf utterance) const override;

private:
    ELanguage Lang;
};

class TSeparatePunctuation : public IUtteranceTransform {
public:
    TString Transform(TStringBuf utterance) const override;
};

class TRemovePunctuation : public IUtteranceTransform {
public:
    TString Transform(TStringBuf utterance) const override;
};

class TLimitNumTokens : public IUtteranceTransform {
public:
    explicit TLimitNumTokens(size_t maxNumTokens);
    TString Transform(TStringBuf utterance) const override;

private:
    const size_t MaxNumTokens;
};

class TReplacePunct4Insight : public IUtteranceTransform {
public:
    TReplacePunct4Insight();
    TString Transform(TStringBuf utterance) const override;

private:
    THashMap<char, TString> Dict;
};

class TMapYo : public IUtteranceTransform {
public:
    TString Transform(TStringBuf utterance) const override;

private:
    static const TVector<TString> From;
    static const TString To;
};

class TCompoundUtteranceTransform : public IUtteranceTransform {
public:
    explicit TCompoundUtteranceTransform(TVector<IUtteranceTransformPtr> transforms);
    TString Transform(TStringBuf utterance) const override;

private:
    TVector<IUtteranceTransformPtr> Transforms;
};

class TNlgSearchUtteranceTransform : public IUtteranceTransform {
public:
    explicit TNlgSearchUtteranceTransform(ELanguage lang);
    TString Transform(TStringBuf utterance) const override;

private:
    const TLowerCase LowerCase;
    const TSeparatePunctuation SeparatePunct;
    const TMapYo MapYo;
    const TLimitNumTokens LimitNumTokens;

    static const size_t MaxNumTokens;
};

}

#pragma once

#include <util/generic/string.h>
#include <util/folder/path.h>
#include <util/generic/ptr.h>
#include <util/generic/hash.h>

namespace NNlgServer {

class IContextTransform : public TThrRefBase {
public:
    virtual ~IContextTransform() = default;
    virtual TUtf16String Transform(TWtringBuf context) const = 0;
};
using IContextTransformPtr = TIntrusivePtr<IContextTransform>;

class TSeparatePunctuation : public IContextTransform {
public:
    TUtf16String Transform(TWtringBuf context) const override;
};

class TLimitNumTokens : public IContextTransform {
public:
    explicit TLimitNumTokens(ui64 maxNumTokens);
    TUtf16String Transform(TWtringBuf context) const override;

private:
    const ui64 MaxNumTokens;
};

class TAddCartman : public IContextTransform {
public:
    TUtf16String Transform(TWtringBuf context) const override;
};

class TAddKyleAndCartman : public IContextTransform {
public:
    TUtf16String Transform(TWtringBuf context) const override;
};

class TTranslateWithDict : public IContextTransform {
public:
    TTranslateWithDict(const TFsPath &path);
    TUtf16String Transform(TWtringBuf context) const override;

private:
    THashMap<TUtf16String, TUtf16String> Dict;
};

/*class TReplaceUnknownTokens : public IContextTransform {
public:
    TReplaceUnknownTokens(TTokenDictPtr dict);
    TUtf16String Transform(TWtringBuf context) const override;

private:
    TTokenDictPtr Dict;
};*/

class TCompoundTransform : public IContextTransform {
public:
    TCompoundTransform(TVector<IContextTransformPtr> transforms);
    TUtf16String Transform(TWtringBuf context) const override;

private:
    TVector<IContextTransformPtr> Transforms;
};

}

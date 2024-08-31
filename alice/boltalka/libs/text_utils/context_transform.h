#pragma once
#include "utterance_transform.h"

#include <util/generic/string.h>
#include <util/generic/ptr.h>
#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/generic/vector.h>

namespace NNlgTextUtils {

class IContextTransform : public TThrRefBase {
public:
    virtual ~IContextTransform() = default;
    // Assumes that context of dialog looks like this:
    // ...
    // user:  context[2]
    // alice: context[1]
    // user:  context[0]
    virtual TVector<TString> Transform(TVector<TString> context) const = 0;
};
using IContextTransformPtr = TIntrusivePtr<IContextTransform>;

class TUtteranceWiseTransform : public IContextTransform {
public:
    explicit TUtteranceWiseTransform(IUtteranceTransformPtr transform);
    TVector<TString> Transform(TVector<TString> context) const override;

private:
    IUtteranceTransformPtr UtteranceTransform;
};

class TSetContextNumTurns : public IContextTransform {
public:
    explicit TSetContextNumTurns(size_t numTurns);
    TVector<TString> Transform(TVector<TString> context) const override;

private:
    const size_t NumTurns;
};

class TCutAliceFromUser : public IContextTransform {
public:
    TVector<TString> Transform(TVector<TString> context) const override;

private:
    static const THashSet<TString> Alices;
};

class TCompoundContextTransform : public IContextTransform {
public:
    explicit TCompoundContextTransform(TVector<IContextTransformPtr> transforms);
    TVector<TString> Transform(TVector<TString> context) const override;

private:
    TVector<IContextTransformPtr> Transforms;
};

class TNlgSearchContextTransform : public IContextTransform {
public:
    explicit TNlgSearchContextTransform(ELanguage lang);
    TVector<TString> Transform(TVector<TString> context) const override;

private:
    const TNlgSearchUtteranceTransform UtteranceTransform;
    const TCutAliceFromUser CutAlice{};
};

}

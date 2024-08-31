#pragma once

#include <library/cpp/langs/langs.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/hash.h>

namespace NVins {

struct TSparseFeature {
    TString Value;
    float Weight = 1.f;

    TSparseFeature() = default;

    explicit TSparseFeature(TString value, float weight = 1.)
        : Value(std::move(value))
        , Weight(weight)
    {
    }
};

struct TSample {
    TString Text;
    TVector<TString> Tokens;
    TVector<TString> Tags;
};

struct TSampleFeatures {
    TSample Sample;
    THashMap<TString, TVector<TVector<float>>> DenseSeq;
    THashMap<TString, TVector<TVector<TSparseFeature>>> SparseSeq;
};

TVector<TString> GetTags(const TString& tagging, const TVector<TString>& tokens, ELanguage lang);

} // namespace NVins

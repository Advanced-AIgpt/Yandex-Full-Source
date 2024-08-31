#pragma once

#include "relevance_based.h"

#include <alice/nlu/libs/embedder/embedder.h>
#include <alice/nlu/libs/tf_nn_model/tf_nn_model.h>

#include <util/stream/input.h>
#include <util/generic/hash.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>


namespace NAlice {
namespace NItemSelector {

class INnComputer {
public:
    virtual ~INnComputer() = default;
    virtual float Predict(const TVector<int>& lengths, const TVector<TVector<TVector<float>>>& embeddings) const = 0;
};

class TNnComputer final : public NVins::TTfNnModel, public INnComputer {
public:
    TNnComputer(IInputStream* protobufModel, IInputStream* modelDescription);

    float Predict(const TVector<int>& lengths, const TVector<TVector<TVector<float>>>& embeddings) const override;

private:
    void SaveModelParameters(const TString& /* modelDirName */) const override {}
};

class TLSTMRelevanceComputer final : public IRelevanceComputer {
public:
    TLSTMRelevanceComputer(THolder<const INnComputer> computer, const NAlice::ITokenEmbedder* embedder,
                           const THashMap<TString, TVector<float>>& specialEmbeddings, const size_t maxTokens);

    float ComputeRelevance(const TString& request, const TVector<TString>& synonyms) const;

private:
    TVector<TVector<float>> Embed(const TString& request, const TVector<TString>& synonyms) const;

private:
    const THolder<const INnComputer> Computer;
    const NAlice::ITokenEmbedder* Embedder;
    const THashMap<TString, TVector<float>> SpecialEmbeddings;
    const size_t MaxTokens = 0;
};

THashMap<TString, TVector<float>> ReadSpecialEmbeddings(IInputStream* input);

} // namespace NItemSelector
} // namespace NAlice

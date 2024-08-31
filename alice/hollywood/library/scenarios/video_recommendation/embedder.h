#pragma once

#include <kernel/dssm_applier/nn_applier/lib/layers.h>

#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/memory/blob.h>

namespace NAlice::NHollywood {

class TMovieInfoEmbedder {
public:
    struct TConfig {
        TVector<TString> QueryEmbedderInputs;
        TVector<TString> QueryEmbedderOutputs;
        TVector<TString> DocEmbedderInputs;
        TVector<TString> DocEmbedderOutputs;
    };

    void Load(const TString& embedderPath, const TString& embedderConfigPath);

    TVector<float> EmbedQuery(const TString& query) const;
    TVector<float> EmbedMovie(const TString& title, const TString& host, const TString& path) const;

    static TConfig LoadConfig(IInputStream* jsonConfigStream);

private:
    TConfig Config;
    TIntrusivePtr<NNeuralNetApplier::TModel> QueryEmbedder;
    TIntrusivePtr<NNeuralNetApplier::TModel> DocEmbedder;
};

} // namespace NAlice::NHollywood

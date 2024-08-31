#include "embedder.h"

#include <library/cpp/resource/resource.h>

namespace NAlice::NHollywood {

namespace {

TVector<TString> ReadStringArray(const NJson::TJsonValue& json) {
    TVector<TString> stringArray;
    for (const auto& value : json.GetArray()) {
        stringArray.push_back(value.GetString());
    }
    return stringArray;
}

} // namespace

void TMovieInfoEmbedder::Load(const TString& embedderPath, const TString& embedderConfigPath) {
    TFileInput embedderConfigStream(embedderConfigPath);
    Config = LoadConfig(&embedderConfigStream);

    const TBlob embedderBlob = TBlob::PrechargedFromFile(embedderPath);

    NNeuralNetApplier::TModel model;
    model.Load(embedderBlob);
    model.Init();

    QueryEmbedder = model.GetSubmodel(Config.QueryEmbedderOutputs[0]);
    DocEmbedder = model.GetSubmodel(Config.DocEmbedderOutputs[0]);
}

TMovieInfoEmbedder::TConfig TMovieInfoEmbedder::LoadConfig(IInputStream* jsonConfigStream) {
    Y_ENSURE(jsonConfigStream);

    NJson::TJsonValue jsonConfig;
    const bool readCorrectly = NJson::ReadJsonTree(jsonConfigStream, &jsonConfig);
    Y_ENSURE(readCorrectly);

    TConfig config;
    config.QueryEmbedderInputs = ReadStringArray(jsonConfig["query_inputs"]);
    config.QueryEmbedderOutputs = ReadStringArray(jsonConfig["query_outputs"]);
    config.DocEmbedderInputs = ReadStringArray(jsonConfig["doc_inputs"]);
    config.DocEmbedderOutputs = ReadStringArray(jsonConfig["doc_outputs"]);

    return config;
}

TVector<float> TMovieInfoEmbedder::EmbedQuery(const TString& query) const {
    const TVector<TString> inputs = {query};
    const auto sample = MakeAtomicShared<NNeuralNetApplier::TSample>(Config.QueryEmbedderInputs, inputs);

    TVector<float> embedding;
    QueryEmbedder->Apply(sample, Config.QueryEmbedderOutputs, embedding);

    return embedding;
}

TVector<float> TMovieInfoEmbedder::EmbedMovie(const TString& title, const TString& host, const TString& path) const {
    const TVector<TString> inputs = {title, host, path};
    const auto sample = MakeAtomicShared<NNeuralNetApplier::TSample>(Config.DocEmbedderInputs, inputs);

    TVector<float> embedding;
    DocEmbedder->Apply(sample, Config.DocEmbedderOutputs, embedding);

    return embedding;
}

} // namespace NAlice::NHollywood

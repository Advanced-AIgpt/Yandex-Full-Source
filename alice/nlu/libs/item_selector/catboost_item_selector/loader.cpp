#include "loader.h"

#include <catboost/libs/model/model.h>

#include <library/cpp/json/json_reader.h>

#include <util/stream/file.h>

namespace {

NVins::TRnnTagger LoadRnnTagger(const TString& taggerPath) {
    return NVins::TRnnTagger(taggerPath);
}

NAlice::TTokenEmbedder LoadTokenEmbedder(const TString& embeddingsPath, const TString& dictionaryPath) {
    return NAlice::TTokenEmbedder(TBlob::PrechargedFromFile(embeddingsPath), TBlob::PrechargedFromFile(dictionaryPath));
}

} // anonymous namespace

namespace NAlice {
namespace NItemSelector {

TCatboostItemSelectorSpec ReadSpec(const TString& path) {
    TCatboostItemSelectorSpec spec;
    TFileInput in(path);
    const auto jsonSpec = NJson::ReadJsonTree(&in, /* throwOnError= */ true);
    for (const auto& [key, value] : jsonSpec["features"].GetMapSafe()) {
        spec.FeatureSpec[key] = value.GetIntegerSafe();
    }
    spec.FeatureVectorSize = jsonSpec["feature_vector_size"].GetIntegerSafe();
    spec.SelectionThreshold = jsonSpec["threshold"].GetDoubleSafe();
    spec.RemoveBioPrefix = jsonSpec["remove_bio_prefix"].GetBooleanSafe();
    return spec;
}

TCatboostItemSelector LoadModel(const TString& modelPath, const TString& specPath) {
    return {ReadModel(modelPath), ReadSpec(specPath)};
}

TEasyTagger GetEasyTagger(const TString& taggerPath, const TString& embeddingsPath,
                          const TString& dictionaryPath, const size_t topSize) {
    return {LoadRnnTagger(taggerPath), LoadTokenEmbedder(embeddingsPath, dictionaryPath), topSize};
}

} // namespace NItemSelector
} // namespace NAlice

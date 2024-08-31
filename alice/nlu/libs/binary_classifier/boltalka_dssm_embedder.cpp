#include "boltalka_dssm_embedder.h"

#include <library/cpp/json/json_reader.h>
#include <library/cpp/langs/langs.h>

namespace {

TVector<TString> ReadStringArray(const NJson::TJsonValue& json) {
    TVector<TString> stringArray;
    for (const auto& value : json.GetArraySafe()) {
        stringArray.push_back(value.GetStringSafe());
    }
    return stringArray;
}

TVector<size_t> ReadIntegerArray(const NJson::TJsonValue& json) {
    TVector<size_t> intArray;
    for (const auto& value : json.GetArraySafe()) {
        intArray.push_back(value.GetIntegerSafe());
    }
    return intArray;
}

ELanguage ReadLanguageSafe(const NJson::TJsonValue& json) {
    if (const auto* value = json.GetValueByPath("language")) {
        const auto& language = value->GetString();
        if (!language.empty()) {
            return LanguageByName(language);
        }
    }
    return LANG_RUS;
}

NNeuralNetApplier::TModel LoadModel(const TBlob& modelBlob) {
    NNeuralNetApplier::TModel model;
    model.Load(modelBlob);
    model.Init();

    return model;
}

} // namespace

namespace NAlice {

TBoltalkaDssmEmbedder::TBoltalkaDssmEmbedder(const TBlob& modelBlob, IInputStream* jsonConfigStream)
    : TBoltalkaDssmEmbedder(TBoltalkaDssmEmbedder(modelBlob, LoadConfig(jsonConfigStream)))
{
}

TBoltalkaDssmEmbedder::TBoltalkaDssmEmbedder(const TBlob& modelBlob, const TConfig& config)
    : Model(LoadModel(modelBlob))
    , Transform(MakeIntrusive<NNlgTextUtils::TNlgSearchUtteranceTransform>(config.Language))
    , Config(config)
{
}

TVector<float> TBoltalkaDssmEmbedder::Embed(const TStringBuf query) const {
    const TVector<TString> modelFeed = { Transform->Transform(query) };
    const auto sample = MakeAtomicShared<NNeuralNetApplier::TSample>(Config.Inputs, modelFeed);

    TVector<float> embedding;
    Model.Apply(sample, Config.Outputs, embedding);

    return embedding;
}

TVector<TVector<float>> TBoltalkaDssmEmbedder::Embed(const TVector<TString>& inputs) const {
    Y_ENSURE(inputs.size() == Config.Inputs.size());
    TVector<TString> modelFeed;
    modelFeed.reserve(inputs.size());
    for (const auto& input : inputs){
        modelFeed.push_back(Transform->Transform(input));
    }
    const auto sample = MakeAtomicShared<NNeuralNetApplier::TSample>(Config.Inputs, modelFeed);

    TVector<float> embedding;
    Model.Apply(sample, Config.Outputs, embedding);
    TVector<TVector<float>> result;
    result.reserve(Config.Outputs.size());
    size_t beginIndex = 0;
    for (size_t i = 0; i < Config.Outputs.size(); ++i) {
        result.emplace_back(embedding.begin() + beginIndex, embedding.begin() + beginIndex + Config.OutputVectorSizes[i]);
        beginIndex += Config.OutputVectorSizes[i];
    }
    Y_ASSERT(beginIndex == embedding.size());
    return result;
}

TBoltalkaDssmEmbedder::TConfig TBoltalkaDssmEmbedder::LoadConfig(IInputStream* jsonConfigStream) {
    Y_ENSURE(jsonConfigStream);

    NJson::TJsonValue jsonConfig;
    const bool readCorrectly = NJson::ReadJsonTree(jsonConfigStream, &jsonConfig);
    Y_ENSURE(readCorrectly);

    TConfig config;

    config.Inputs = ReadStringArray(jsonConfig["inputs"]);
    config.Outputs = ReadStringArray(jsonConfig["outputs"]);
    config.Language = ReadLanguageSafe(jsonConfig);
    config.OutputVectorSizes = ReadIntegerArray(jsonConfig["output_vector_sizes"]);
    Y_ENSURE(config.Outputs.size() == config.OutputVectorSizes.size());

    return config;
}

} //namespace NAlice

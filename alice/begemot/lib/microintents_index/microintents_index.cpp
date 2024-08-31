#include "microintents_index.h"

#include <library/cpp/dot_product/dot_product.h>
#include <library/cpp/yaml/as/tstring.h>

namespace NBg {

    static constexpr float UNDEFINED_SCORE = -2.f;
    static constexpr size_t MAX_NUM_TOKENS = 100;

    static const TVector<TString> MODEL_INPUTS = { "reply" };
    static const TVector<TString> MODEL_OUTPUTS = { "doc_embedding" };

    static NNeuralNetApplier::TModel LoadModel(TBlob& modelBlob) {
        NNeuralNetApplier::TModel model;
        model.Load(modelBlob);
        model.Init();

        return model;
    }

    static NNlgTextUtils::IUtteranceTransformPtr MakeTransform(ELanguage languageId) {
        return new NNlgTextUtils::TCompoundUtteranceTransform({
            new NNlgTextUtils::TLowerCase(languageId),
            new NNlgTextUtils::TRemovePunctuation(),
            new NNlgTextUtils::TLimitNumTokens(MAX_NUM_TOKENS)
        });
    }

    TMicrointentsIndex::TMicrointentsIndex(TBlob& modelBlob, const TString& config, ELanguage languageId)
        : Model(LoadModel(modelBlob))
        , Transform(MakeTransform(languageId))
    {
        ReadConfig(config);
    }

    void TMicrointentsIndex::ReadConfig(const TString& config) {
        YAML::Node rootNode = YAML::Load(config);

        const float defaultThreshold = rootNode["default_threshold"].as<float>();
        for (const auto& intentInfoNode : rootNode["intents"]) {
            const TString intent = intentInfoNode.first.as<TString>();

            float threshold = defaultThreshold;
            if (intentInfoNode.second["threshold"].IsDefined()) {
                threshold = intentInfoNode.second["threshold"].as<float>();
            }
            Intents.push_back({intent, threshold});

            const TIntentId intentId = static_cast<TIntentId>(Intents.size() - 1);

            for (const auto& textNode : intentInfoNode.second["nlu"]) {
                Elements.push_back({GetEmbedding(textNode.as<TString>()), intentId});
            }
        }
    }

    bool TMicrointentsIndex::TryPredictIntent(TStringBuf query, TString* predictedIntent, float* predictedScore) const {
        const size_t nearestNeighbourIndex = FindNearestNeighbour(query, predictedScore);

        if (nearestNeighbourIndex != NPOS) {
            *predictedIntent = GetIntentInfo(nearestNeighbourIndex).Name;
            return true;
        }
        return false;
    }

    TVector<float> TMicrointentsIndex::GetEmbedding(TStringBuf phrase) const {
        const TVector<TString> modelFeed = { Transform->Transform(phrase) };
        const auto sample = MakeAtomicShared<NNeuralNetApplier::TSample>(MODEL_INPUTS, modelFeed);

        TVector<float> embedding;
        Model.Apply(sample, MODEL_OUTPUTS, embedding);

        return embedding;
    }

    size_t TMicrointentsIndex::FindNearestNeighbour(TStringBuf query, float* nearestNeighbourScore) const {
        const TVector<float> queryEmbedding = GetEmbedding(query);

        *nearestNeighbourScore = UNDEFINED_SCORE;
        size_t nearestNeighbourIndex = NPOS;
        for (size_t elementIndex = 0; elementIndex < Elements.size(); ++elementIndex) {
            const auto& elementEmbedding = Elements[elementIndex].Embedding;
            Y_ENSURE(elementEmbedding.size() == queryEmbedding.size());

            const float score = DotProduct(elementEmbedding.data(), queryEmbedding.data(), queryEmbedding.size());

            if (score > *nearestNeighbourScore && score > GetIntentInfo(elementIndex).Threshold) {
                nearestNeighbourIndex = elementIndex;
                *nearestNeighbourScore = score;
            }
        }

        return nearestNeighbourIndex;
    }

} // namespace NBg

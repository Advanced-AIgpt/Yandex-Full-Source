#pragma once

#include <alice/nlu/libs/anaphora_resolver/common/match.h>
#include <alice/nlu/libs/anaphora_resolver/common/mention.h>
#include <alice/nlu/libs/embedder/embedder.h>
#include <alice/nlu/libs/sample_features/sample_features.h>
#include <alice/nlu/libs/tf_nn_model/tf_nn_model.h>

namespace NAlice {
    class TLstmAnaphoraMatcherModel : public NVins::TTfNnModel {
    public:
        struct TModelDescription {
            TVector<TString> Inputs;
            TVector<TString> Outputs;
            double NoAnaphoraProbabilityThreshold = 0.5;
            double AnaphoraInRequestProbabilityThreshold = 0.5;
            double EntityProbabilityThreshold = 0.5;
            size_t MaxHistoryLength = 5;
        };

        using TSpecialTokenEmbeddings = THashMap<TString, TVector<float>>;

    public:
        TLstmAnaphoraMatcherModel(IInputStream* protobufModel,
                                  const TModelDescription& modelDescription,
                                  const TSpecialTokenEmbeddings& specialTokenEmbeddings,
                                  const TTokenEmbedder& tokenEmbedder);
        TLstmAnaphoraMatcherModel(const TString& modelDirName, const TTokenEmbedder& tokenEmbedder);

        TMaybe<TAnaphoraMatch> Predict(const TVector<NVins::TSample>& dialogHistory,
                                       const TVector<TMentionInDialogue>& entityPositions,
                                       const TMentionInDialogue& pronounPosition,
                                       const TString& pronounGrammemes = "") const;

        static TModelDescription ReadJsonModelDescription(IInputStream* input);
        static TModelDescription ReadJsonModelDescription(const TString& filePath);
        static TSpecialTokenEmbeddings ReadJsonSpecialTokenEmbeddings(IInputStream* input);
        static TSpecialTokenEmbeddings ReadJsonSpecialTokenEmbeddings(const TString& filePath);

    private:
        void SetFeed(const TVector<NVins::TSample>& dialogHistory,
                     const TVector<TMentionInDialogue>& entityPositions,
                     const TMentionInDialogue& pronounPosition,
                     NNeuralNet::TTensorList* feed) const;

        void SetEmbeddingsAndSegmentIds(const TVector<NVins::TSample>& dialogHistory, NNeuralNet::TTensorList* feed) const;

        void SetSegmentPositions(const TVector<NVins::TSample>& dialogHistory,
                                 const TVector<TMentionInDialogue>& entityPositions,
                                 const TMentionInDialogue& pronounPosition,
                                 NNeuralNet::TTensorList* feed) const;

        void SaveModelParameters(const TString& /*modelDirName*/) const override {
            // TODO(smirnovpavel): remove implementation, move Save and SaveModelParameters to a new class NVins::TSaveableTfModel
        }

    private:
        TModelDescription ModelDescription;
        TSpecialTokenEmbeddings SpecialTokenEmbeddings;
        TTokenEmbedder TokenEmbedder;
    };
} // namespace NAlice

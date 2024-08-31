#pragma once

#include <alice/boltalka/libs/text_utils/utterance_transform.h>
#include <kernel/dssm_applier/nn_applier/lib/layers.h>
#include <library/cpp/langs/langs.h>
#include <util/stream/input.h>

namespace NBg {

    // Storage for the Alice microintents in the DSSM-based index
    class TMicrointentsIndex {
    public:
        using TIntentId = ui16;

        struct TIntentInfo {
            TString Name;
            float Threshold;
        };

        struct TIndexElement {
            TVector<float> Embedding;
            TIntentId IntentId;
        };

    public:
        TMicrointentsIndex(TBlob& modelBlob, const TString& config, ELanguage languageId);

        // Returns true if the index contains an element with similarity to the query greater than the threshold.
        //  In this case returns also the name of the intent corresponding to the most similar element in the index
        //  and the similarity score.
        // Otherwise returns false.
        bool TryPredictIntent(TStringBuf query, TString* predictedIntent, float* predictedScore) const;

    private:
        void ReadConfig(const TString& config);

        const TIntentInfo& GetIntentInfo(size_t elementIndex) const {
            return Intents[Elements[elementIndex].IntentId];
        }

        TVector<float> GetEmbedding(TStringBuf phrase) const;

        size_t FindNearestNeighbour(TStringBuf query, float* nearestNeighbourScore) const;

    private:
        NNeuralNetApplier::TModel Model;
        NNlgTextUtils::IUtteranceTransformPtr Transform;

        TVector<TIntentInfo> Intents;
        TVector<TIndexElement> Elements;
    };

    using TMicrointentsIndexPtr = THolder<TMicrointentsIndex>;

} // namespace NBg

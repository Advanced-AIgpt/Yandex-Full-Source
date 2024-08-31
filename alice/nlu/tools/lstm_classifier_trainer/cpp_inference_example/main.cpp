#include "util/generic/yexception.h"
#include <alice/nlu/libs/embedder/embedder.h>
#include <alice/nlu/libs/encoder/encoder.h>

#include <library/cpp/resource/resource.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/memory/blob.h>
#include <util/string/split.h>

NAlice::TTokenEmbedder GetTokenEmbedder() {
    return {TBlob::PrechargedFromFile("model/embeddings"),
            TBlob::PrechargedFromFile("model/embeddings_dictionary.trie")};
}

class TClassifier {
public:
    TClassifier()
        : Embedder(GetTokenEmbedder())
        , Encoder("model")
    {
        Encoder.EstablishSessionIfNotYet();
    }

    TVector<float> PredictProba(const TVector<TString>& texts) const {
        NVins::TEncoderInput input;

        for (const auto& text : texts) {
            const TVector<TString> tokens = StringSplitter(text).Split(' ');
            const TVector<TVector<float>> embeddings = Embedder.EmbedSequence(tokens);

            input.Lengths.push_back(tokens.ysize());
            input.DenseSeq.emplace_back(std::move(embeddings));
        }

        const TVector<TVector<float>> predictions = Encoder.Encode(input);
        Y_ENSURE(predictions.size() == texts.size()); // Batch size
        Y_ENSURE(predictions[0].size() == 1); // Number of classes (binary classification in case of this model)

        TVector<float> positiveProbs;
        for (const auto& prediction : predictions) {
            positiveProbs.push_back(prediction[0]);
        }

        return positiveProbs;
    }

private:
    NAlice::TTokenEmbedder Embedder;
    NVins::TEncoder Encoder;
};

int main() {
    TClassifier classifier;

    const TVector<TString> texts = {"алиса включи музыку", "мама мыла раму"};
    const TVector<float> probs = classifier.PredictProba(texts);

    for (size_t i = 0; i < texts.size(); ++i) {
        Cerr << texts[i] << " - " << probs[i] << Endl;
    }

    Y_ENSURE(probs.size() == texts.size());
    Y_ENSURE(probs[0] > 0.9);
    Y_ENSURE(probs[1] < 0.1);

    return 0;
}

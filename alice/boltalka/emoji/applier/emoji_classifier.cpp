#include "emoji_classifier.h"

#include <library/cpp/json/json_reader.h>
#include <util/generic/algorithm.h>

namespace NNlg {

namespace {

TVector<TString> LoadConfig(IInputStream* jsonConfigStream) {
    Y_ENSURE(jsonConfigStream);

    NJson::TJsonValue jsonConfig;
    const bool readCorrectly = NJson::ReadJsonTree(jsonConfigStream, &jsonConfig);
    Y_ENSURE(readCorrectly);

    TVector<TString> emojis;
    for (const auto& node : jsonConfig.GetArraySafe()) {
        emojis.emplace_back(node.GetStringSafe());
    }

    return emojis;
}

} // namespace


TEmojiClassifier::TEmojiClassifier(IInputStream* model, IInputStream* modelConfig, IInputStream* emojiConfig)
    : Classifier(model, modelConfig)
    , Emojis(LoadConfig(emojiConfig))
{
    Classifier.Predict(TVector<float>(Classifier.GetEmbeddingSize(), 0.0), TVector<float>(Classifier.GetEmbeddingSize(), 0.0));
}

TMaybe<TString> TEmojiClassifier::Predict(const TVector<float>& queryEmbedding, const TVector<float>& replyEmbedding) const {
    const auto scores = Classifier.Predict(queryEmbedding, replyEmbedding);
    Y_ENSURE(scores.size() == Emojis.size());
    auto prediction = std::distance(scores.begin(), MaxElement(scores.begin(), scores.end()));

    if (prediction == 0) {
        return Nothing();
    }

    return Emojis[prediction];
}

} // namespace NNlg

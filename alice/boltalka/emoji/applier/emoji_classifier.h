#pragma once

#include "model.h"

namespace NNlg {

class TEmojiClassifier {
public:
    TEmojiClassifier(IInputStream* model, IInputStream* modelConfig, IInputStream* emojiConfig);

    TMaybe<TString> Predict(const TVector<float>& queryEmbedding, const TVector<float>& replyEmbedding) const;

private:
    TEmojiClassifierModel Classifier;
    TVector<TString> Emojis;
};

} // namespace NAlice::NHollywood::NGeneralConversation

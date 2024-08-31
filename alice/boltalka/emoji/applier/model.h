#pragma once

#include <alice/nlu/libs/tf_nn_model/tf_nn_model.h>

namespace NNlg {

class TEmojiClassifierModel: public NVins::TTfNnModel {
public:
    struct TConfig {
        TString InputNode;
        TString OutputNode;
        size_t EmbeddingSize = 0;
    };

    TEmojiClassifierModel(IInputStream* model, IInputStream* jsonConfigStream);

    TVector<float> Predict(const TVector<float>& queryEmbedding, const TVector<float>& replyEmbedding) const;
    size_t GetEmbeddingSize() const;

    void SaveModelParameters(const TString& /*modelDirName*/) const override {
    }

private:
    TConfig Config;
};

} // namespace NNlg

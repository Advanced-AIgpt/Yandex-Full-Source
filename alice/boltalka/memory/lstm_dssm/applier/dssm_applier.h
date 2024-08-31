#pragma once

// Fix tensorflow LOG macro leakage
#pragma push_macro("LOG")
#include <alice/nlu/libs/tf_nn_model/tf_nn_model.h>
#include <alice/nlu/libs/tf_nn_model/tensor_helpers.h>
#pragma pop_macro("LOG")

namespace NNlg {

class TDSSMMemoryApplier: public NVins::TTfNnModel {
public:
    struct TConfig {
        TVector<TString> InputNodes;
        TString OutputNode;
        size_t StateSize = 0;
        size_t EmbeddingSize = 0;
    };

    TDSSMMemoryApplier(IInputStream* model, IInputStream* jsonConfigStream);
    TDSSMMemoryApplier(IInputStream* model, const TConfig& config);

    TVector<float> Predict(const TVector<float>& state, const TVector<TVector<float>>& replyEmbeddings) const;

    void SaveModelParameters(const TString& /*modelDirName*/) const override {
    }

    size_t GetStateSize() const;
    size_t GetEmbeddingSize() const;

private:
    TConfig Config;
};

} // namespace NNlg

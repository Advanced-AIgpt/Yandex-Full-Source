#pragma once

#include <alice/nlu/libs/tf_nn_model/tf_nn_model.h>
#include <alice/nlu/libs/tf_nn_model/tensor_helpers.h>

namespace NNlg {

class TLSTMMemoryApplier : public NVins::TTfNnModel {
public:
    struct TConfig {
        TVector<TString> InputNodes;
        TString OutputNode;
        size_t StateSize = 0;
    };

    TLSTMMemoryApplier(IInputStream* model, IInputStream* jsonConfigStream);

    TLSTMMemoryApplier(IInputStream* model, const TConfig& config);

    TVector<TVector<float>> Predict(const TVector<float>& currentEmbedding, const TVector<TVector<float>>& currentState) const;

    void SaveModelParameters(const TString& /*modelDirName*/) const override {
    }

    size_t GetStateSize() const;

private:
    size_t StateSize;
};

} // namespace NNlg

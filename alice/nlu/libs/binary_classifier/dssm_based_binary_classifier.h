#pragma once

#include <alice/nlu/libs/tf_nn_model/tf_nn_model.h>

#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/stream/input.h>

namespace NAlice {

class TDssmBasedBinaryClassifier : public NVins::TTfNnModel {
public:
    struct TConfig {
        TString InputNode;
        TString OutputNode;
        size_t InputVectorSize = 0;
    };

    TDssmBasedBinaryClassifier(IInputStream* protobufModelStream, IInputStream* jsonConfigStream);
    TDssmBasedBinaryClassifier(IInputStream* protobufModelStream, const TConfig& config);

    virtual float PredictProbability(const TVector<float>& queryEmbedding) const;

    static TConfig LoadConfig(IInputStream* jsonConfigStream);

protected:
    void SaveModelParameters(const TString& /* modelDirName */) const override {
        // TODO(DIALOG-5871): Remove SaveModelParameters from the TTfNNModel's interface
    }

    NNeuralNet::TTensorList GetModelFeed(const TVector<float>& queryEmbedding) const;

    const TConfig& GetConfig() const;

private:
    TConfig Config;
};

using TDssmBasedBinaryClassifierPtr = THolder<TDssmBasedBinaryClassifier>;

} // namespace NAlice

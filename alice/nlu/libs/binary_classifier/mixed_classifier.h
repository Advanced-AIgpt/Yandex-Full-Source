#pragma once

#include <alice/nlu/libs/binary_classifier/mixed_input.h>
#include <alice/nlu/libs/binary_classifier/proto/model_description.pb.h>

#include <alice/nlu/libs/tf_nn_model/tf_nn_model.h>

#include <util/generic/hash.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/stream/input.h>

namespace NAlice {

class TMixedBinaryClassifier : public NVins::TTfNnModel {
public:
    TMixedBinaryClassifier(IInputStream* protobufModelStream, IInputStream* configStream);

    const TBinaryClassifierModelDescription& GetConfig() const;

    bool NeedSentenceEmebedding(TStringBuf embeddingName) const;
    bool NeedSentenceFeatures() const;

    float Predict(const TMixedBinaryClassifierInput& input) const;

protected:
    void SaveModelParameters(const TString& /* modelDirName */) const override {
        Y_ENSURE(false, "Not supported, see DIALOG-5871");
    }

private:
    void InitializeFeatureNameToIndex();
    NNeuralNet::TTensorList CreateModelInput(const TMixedBinaryClassifierInput& input) const;
    TVector<float> GetEmbeddingVector(const TMixedBinaryClassifierInput& input) const;
    TVector<float> CreateFeatureVector(const TMixedBinaryClassifierInput& input) const;

private:
    TBinaryClassifierModelDescription Config;
    THashMap<TString, size_t> FeatureNameToIndex;
};

using TMixedBinaryClassifierPtr = THolder<TMixedBinaryClassifier>;

} // namespace NAlice

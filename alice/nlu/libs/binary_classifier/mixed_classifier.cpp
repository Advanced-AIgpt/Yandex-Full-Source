#include "mixed_classifier.h"

#include <alice/nlu/libs/binary_classifier/model_description.h>

#include <alice/nlu/granet/lib/utils/utils.h>
#include <alice/nlu/libs/tf_nn_model/tensor_helpers.h>

#include <library/cpp/iterator/enumerate.h>
#include <util/string/split.h>

namespace NAlice {

TMixedBinaryClassifier::TMixedBinaryClassifier(
        IInputStream* protobufModelStream,
        IInputStream* configStream)
    : NVins::TTfNnModel(protobufModelStream)
    , Config(LoadBinaryClassifierModelDescription(configStream))
{
    InitializeFeatureNameToIndex();
    SetInputNodes({Config.GetInputNode()});
    SetOutputNodes({Config.GetOutputNode()});
    EstablishSessionIfNotYet();
}

void TMixedBinaryClassifier::InitializeFeatureNameToIndex() {
    for (const auto& [index, name] : Enumerate(Config.GetInputDescription().GetSentence().GetFeatures())) {
        FeatureNameToIndex[name] = index;
    }
}

const TBinaryClassifierModelDescription& TMixedBinaryClassifier::GetConfig() const {
    return Config;
}

bool TMixedBinaryClassifier::NeedSentenceEmebedding(TStringBuf embeddingName) const {
    return Config.GetInputDescription().GetSentence().GetEmbedding() == embeddingName;
}

bool TMixedBinaryClassifier::NeedSentenceFeatures() const {
    return Config.GetInputDescription().GetSentence().GetFeatures().size() > 0;
}

float TMixedBinaryClassifier::Predict(const TMixedBinaryClassifierInput& input) const {
    Y_ENSURE(IsSessionEstablished(), "Session is not established");

    NNeuralNet::TTfWorkerPtr worker = CreateWorker();
    const NNeuralNet::TTensorList modelInput = CreateModelInput(input);
    const NNeuralNet::TTensorList modelOutputs = worker->Process(modelInput);

    Y_ENSURE(modelOutputs.size() == 1, "Invalid model output size: " << modelOutputs.size());
    Y_ENSURE(modelOutputs[0].dims() == 1, "Invalid output dims: " << modelOutputs[0].dims());
    Y_ENSURE(modelOutputs[0].dim_size(0) == 1, "Invalid batch size: " << modelOutputs[0].dim_size(0));

    return modelOutputs[0].tensor<float, 1>()(0);
}

NNeuralNet::TTensorList TMixedBinaryClassifier::CreateModelInput(const TMixedBinaryClassifierInput& input) const {
    const size_t size = Config.GetInputDescription().GetSentence().GetVectorSize();
    TVector<float> vector(Reserve(size));
    NGranet::Extend(GetEmbeddingVector(input), &vector);
    NGranet::Extend(CreateFeatureVector(input), &vector);

    Y_ENSURE(vector.size() == size, "Invalid input vector size: expected " << size << ", got " << vector.size());
    return { NVins::Convert2DimTableToTensor<float>({vector}) };
}

TVector<float> TMixedBinaryClassifier::GetEmbeddingVector(const TMixedBinaryClassifierInput& input) const {
    const TString name = Config.GetInputDescription().GetSentence().GetEmbedding();
    if (name.empty()) {
        return {};
    }
    const TVector<float>* embedding = input.SentenceEmbeddings.FindPtr(name);
    Y_ENSURE(embedding);
    return *embedding;
}

TVector<float> TMixedBinaryClassifier::CreateFeatureVector(const TMixedBinaryClassifierInput& input) const {
    TVector<float> features(Config.GetInputDescription().GetSentence().GetFeatures().size());
    for (const auto& [name, value] : input.SentenceFeatures) {
        const size_t* index = FeatureNameToIndex.FindPtr(name);
        if (index == nullptr) {
            continue;
        }
        Y_ENSURE(*index < features.size());
        features[*index] = value;
    }
    return features;
}

} // namespace NAlice

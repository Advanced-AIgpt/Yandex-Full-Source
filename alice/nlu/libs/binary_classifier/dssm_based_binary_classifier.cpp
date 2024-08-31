#include "dssm_based_binary_classifier.h"

#include <alice/nlu/libs/tf_nn_model/tensor_helpers.h>

#include <library/cpp/json/json_reader.h>

namespace NAlice {

TDssmBasedBinaryClassifier::TDssmBasedBinaryClassifier(IInputStream* protobufModelStream,
                                                       IInputStream* jsonConfigStream)
    : TDssmBasedBinaryClassifier(protobufModelStream, LoadConfig(jsonConfigStream))
{
}

TDssmBasedBinaryClassifier::TDssmBasedBinaryClassifier(IInputStream* protobufModelStream,
                                                       const TConfig& config)
    : NVins::TTfNnModel(protobufModelStream)
    , Config(config)
{
    SetInputNodes({Config.InputNode});
    SetOutputNodes({Config.OutputNode});
    EstablishSessionIfNotYet();
}

float TDssmBasedBinaryClassifier::PredictProbability(const TVector<float>& queryEmbedding) const {
    Y_ENSURE(IsSessionEstablished(), "Session is not established");
    Y_ENSURE(queryEmbedding.size() == Config.InputVectorSize,
             "Invalid input vector size: expected " << Config.InputVectorSize
             << ", got " << queryEmbedding.size());

    auto worker = CreateWorker();
    const auto modelFeed = GetModelFeed(queryEmbedding);
    const auto modelOutputs = worker->Process(modelFeed);

    Y_ENSURE(modelOutputs.size() == 1, "Invalid model output size: " << modelOutputs.size());
    Y_ENSURE(modelOutputs[0].dims() == 1, "Invalid output dims: " << modelOutputs[0].dims());
    Y_ENSURE(modelOutputs[0].dim_size(0) == 1, "Invalid batch size: " << modelOutputs[0].dim_size(0));

    return modelOutputs[0].tensor<float, 1>()(0);
}

NNeuralNet::TTensorList TDssmBasedBinaryClassifier::GetModelFeed(const TVector<float>& queryEmbedding) const {
    return { NVins::Convert2DimTableToTensor<float>({queryEmbedding}) };
}

TDssmBasedBinaryClassifier::TConfig TDssmBasedBinaryClassifier::LoadConfig(IInputStream* jsonConfigStream) {
    Y_ENSURE(jsonConfigStream);

    NJson::TJsonValue jsonConfig;
    const bool readCorrectly = NJson::ReadJsonTree(jsonConfigStream, &jsonConfig);
    Y_ENSURE(readCorrectly);

    TConfig config;

    config.InputNode = jsonConfig["input_node"].GetStringSafe();
    config.OutputNode = jsonConfig["output_node"].GetStringSafe();
    config.InputVectorSize = jsonConfig["input_vector_size"].GetIntegerSafe();

    return config;
}

const TDssmBasedBinaryClassifier::TConfig& TDssmBasedBinaryClassifier::GetConfig() const {
    return Config;
}

} // namespace NAlice

#include "lstm_applier.h"

#include <library/cpp/json/json_reader.h>

namespace NNlg {

namespace {

TLSTMMemoryApplier::TConfig LoadConfig(IInputStream* jsonConfigStream) {
    Y_ENSURE(jsonConfigStream);

    NJson::TJsonValue jsonConfig;
    const bool readCorrectly = NJson::ReadJsonTree(jsonConfigStream, &jsonConfig);
    Y_ENSURE(readCorrectly);

    TLSTMMemoryApplier::TConfig config;
    for (const auto& node : jsonConfig["input_nodes"].GetArraySafe()) {
        config.InputNodes.emplace_back(node.GetStringSafe());
    }
    config.OutputNode = jsonConfig["output_node"].GetStringSafe();
    config.StateSize = jsonConfig["state_size"].GetUIntegerSafe();

    return config;
}

} // namespace

TLSTMMemoryApplier::TLSTMMemoryApplier(IInputStream* model, const TConfig& config)
    : TTfNnModel(model)
    , StateSize(config.StateSize)
{
    TVector<TString> inputNodes = config.InputNodes;
    SetInputNodes(std::move(inputNodes));
    SetOutputNodes({config.OutputNode});
    EstablishSessionIfNotYet();
}

TLSTMMemoryApplier::TLSTMMemoryApplier(IInputStream* model, IInputStream* jsonConfigStream)
    : TLSTMMemoryApplier(model, LoadConfig(jsonConfigStream))
{
}

TVector<TVector<float>> TLSTMMemoryApplier::Predict(const TVector<float>& currentEmbedding, const TVector<TVector<float>>& currentState) const {
    NNeuralNet::TTensorList inputs;
    inputs.push_back(NVins::Convert1DimTableToTensor<float>(currentEmbedding));
    inputs.push_back(NVins::Convert2DimTableToTensor<float>(currentState));
    const auto result = CreateWorker()->Process(inputs);
    return NVins::ConvertTensorTo2DimTable<float>(result[0]);
}

size_t TLSTMMemoryApplier::GetStateSize() const {
    return StateSize;
}

} // namespace NNlg

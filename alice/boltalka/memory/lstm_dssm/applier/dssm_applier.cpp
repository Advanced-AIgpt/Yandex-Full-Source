#include "dssm_applier.h"

#include <library/cpp/json/json_reader.h>

namespace NNlg {

namespace {

TDSSMMemoryApplier::TConfig LoadConfig(IInputStream* jsonConfigStream) {
    Y_ENSURE(jsonConfigStream);

    NJson::TJsonValue jsonConfig;
    const bool readCorrectly = NJson::ReadJsonTree(jsonConfigStream, &jsonConfig);
    Y_ENSURE(readCorrectly);

    TDSSMMemoryApplier::TConfig config;
    for (const auto& node : jsonConfig["input_nodes"].GetArraySafe()) {
        config.InputNodes.emplace_back(node.GetStringSafe());
    }
    config.OutputNode = jsonConfig["output_node"].GetStringSafe();
    config.StateSize = jsonConfig["state_size"].GetUIntegerSafe();
    config.EmbeddingSize = jsonConfig["embedding_size"].GetUIntegerSafe();

    return config;
}

}

TDSSMMemoryApplier::TDSSMMemoryApplier(IInputStream* model, const TConfig& config)
    : TTfNnModel(model)
    , Config(config)
{
    SetInputNodes({{config.InputNodes}});
    SetOutputNodes({config.OutputNode});
    EstablishSessionIfNotYet();
}

TDSSMMemoryApplier::TDSSMMemoryApplier(IInputStream* model, IInputStream* jsonConfigStream)
    : TDSSMMemoryApplier(model, LoadConfig(jsonConfigStream))
{
}

TVector<float> TDSSMMemoryApplier::Predict(const TVector<float>& state, const TVector<TVector<float>>& replyEmbeddings) const {
    Y_ENSURE(state.size() == Config.StateSize);

    NNeuralNet::TTensorList inputs;
    inputs.push_back(NVins::Convert1DimTableToTensor<float>(state));
    inputs.push_back(NVins::Convert2DimTableToTensor<float>(replyEmbeddings));
    const auto result = CreateWorker()->Process(inputs);
    return NVins::ConvertTensorTo1DimTable<float>(result[0]);
}

size_t TDSSMMemoryApplier::GetStateSize() const {
    return Config.StateSize;
}

size_t TDSSMMemoryApplier::GetEmbeddingSize() const {
    return Config.EmbeddingSize;
}

} // namespace NNlg

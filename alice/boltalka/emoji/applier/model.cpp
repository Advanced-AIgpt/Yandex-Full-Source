#include "model.h"

#include <alice/nlu/libs/tf_nn_model/tensor_helpers.h>
#include <library/cpp/json/json_reader.h>

namespace NNlg {

namespace {

TEmojiClassifierModel::TConfig LoadConfig(IInputStream* jsonConfigStream) {
    Y_ENSURE(jsonConfigStream);

    NJson::TJsonValue jsonConfig;
    const bool readCorrectly = NJson::ReadJsonTree(jsonConfigStream, &jsonConfig);
    Y_ENSURE(readCorrectly);

    TEmojiClassifierModel::TConfig config;
    config.InputNode = jsonConfig["input_node"].GetStringSafe();
    config.OutputNode = jsonConfig["output_node"].GetStringSafe();
    config.EmbeddingSize = jsonConfig["embedding_size"].GetUIntegerSafe();

    return config;
}

}

TEmojiClassifierModel::TEmojiClassifierModel(IInputStream* model, IInputStream* jsonConfigStream)
    : TTfNnModel(model)
    , Config(LoadConfig(jsonConfigStream))
{
    SetInputNodes({Config.InputNode});
    SetOutputNodes({Config.OutputNode});
    EstablishSessionIfNotYet();
}

TVector<float> TEmojiClassifierModel::Predict(const TVector<float>& queryEmbedding, const TVector<float>& replyEmbedding) const {
    Y_ENSURE(queryEmbedding.size() == Config.EmbeddingSize);
    Y_ENSURE(replyEmbedding.size() == Config.EmbeddingSize);

    NNeuralNet::TTensorList inputs = {tf::Tensor(tf::DT_FLOAT, {1ll, static_cast<long long>(Config.EmbeddingSize * 2)})};
    auto&& input = inputs[0].matrix<float>();
    for (size_t i = 0; i < Config.EmbeddingSize; ++i) {
        input(0, i) = queryEmbedding[i];
        input(0, i + Config.EmbeddingSize) = replyEmbedding[i];
    }

    const auto result = CreateWorker()->Process(inputs);
    return NVins::ConvertTensorTo2DimTable<float>(result[0])[0];
}

size_t TEmojiClassifierModel::GetEmbeddingSize() const {
    return Config.EmbeddingSize;
}

} // namespace NNlg


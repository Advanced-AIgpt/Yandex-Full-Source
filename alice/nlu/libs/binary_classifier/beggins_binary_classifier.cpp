#include "beggins_binary_classifier.h"

namespace NAlice {

float TBegginsTensorflowBinaryClassifier::PredictProbability(const TVector<float>& queryEmbedding) const {
    Y_ENSURE(IsSessionEstablished(), "Session is not established");
    Y_ENSURE(queryEmbedding.size() == GetConfig().InputVectorSize,
             "Invalid input vector size: expected " << GetConfig().InputVectorSize << ", got " << queryEmbedding.size());

    auto worker = CreateWorker();
    const auto modelFeed = GetModelFeed(queryEmbedding);
    const auto modelOutputs = worker->Process(modelFeed);

    Y_ENSURE(modelOutputs.size() == 1, "Invalid model output size: " << modelOutputs.size());
    Y_ENSURE(modelOutputs[0].dims() == 2, "Invalid output dims: " << modelOutputs[0].dims());
    Y_ENSURE(modelOutputs[0].dim_size(0) == 1, "Invalid batch size: " << modelOutputs[0].dim_size(0));

    return modelOutputs[0].tensor<float, 2>()(0);
}

} // namespace NAlice

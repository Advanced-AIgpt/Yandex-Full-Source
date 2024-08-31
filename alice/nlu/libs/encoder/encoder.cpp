#include "encoder.h"

#include <alice/nlu/libs/tf_nn_model/tensor_helpers.h>

#include <util/stream/file.h>

using namespace NVins;
using namespace NNeuralNet;
using namespace tensorflow;

namespace {

TString GetModelDescriptionFileName(const TString& modelDirName) {
    return modelDirName + "/model_description";
}

TTensorList ConvertSparseMatrixToTensors(const TSparseMatrix& sparseMatrix) {
    return {
        Convert2DimTableToTensor(sparseMatrix.Indices),
        Convert1DimTableToTensor(sparseMatrix.Values)
    };
}

} // anonymous

namespace NVins {

void SaveEncoderDescription(
    const TString& modelDirName,
    const TEncoderDescription& encoderDescription
) {
    TFileOutput modelDescriptionFile(GetModelDescriptionFileName(modelDirName));
    encoderDescription.Save(&modelDescriptionFile);
}

TEncoder::TEncoder(const TString& modelDirName)
    : TTfNnModel(modelDirName)
{
    TFileInput modelDescriptionFile(GetModelDescriptionFileName(modelDirName));
    ModelDescription.Load(&modelDescriptionFile);

    InitModel();
}

TEncoder::TEncoder(IInputStream* protobufModelStream, const TEncoderDescription& modelDescription)
    : TTfNnModel(protobufModelStream)
    , ModelDescription(modelDescription)
{
    InitModel();
}

void TEncoder::InitModel() {
    TVector<TString> inputNodes(Reserve(ModelDescription.InputsMapping.size()));
    for (const auto& input : ModelDescription.InputsMapping) {
        for (const auto& inputNodeName : input.second) {
            inputNodes.emplace_back(inputNodeName);
        }
    }

    SetInputNodes(std::move(inputNodes));
    SetOutputNodes({ModelDescription.Output});
}

TVector<TVector<float>> TEncoder::Encode(const TEncoderInput& data) const {
    Y_ENSURE(IsSessionEstablished());

    const auto feed = GetFeed(data);
    auto worker = CreateWorker();
    return ConvertTensorTo2DimTable<float>(worker->Process(feed)[0]);
}

bool TEncoder::UsesFeature(const TString& featureName) {
    return ModelDescription.InputsMapping.contains(featureName);
}

TTensorList TEncoder::GetFeed(const TEncoderInput& data) const {
    TTensorList feed;
    feed.reserve(ModelDescription.InputsMapping.size());
    for (const auto& input : ModelDescription.InputsMapping) {
        for (auto&& feedTensor : GetFeed(data, input.first)) {
            feed.emplace_back(std::move(feedTensor));
        }
    }
    return feed;
}

TTensorList TEncoder::GetFeed(
    const TEncoderInput& data,
    const TString& featureName
) const {
    if (featureName == "lengths") {
        return {Convert1DimTableToTensor(data.Lengths)};
    }
    if (featureName == "sparse_seq_ids") {
        return ConvertSparseMatrixToTensors(data.SparseSeqIds);
    }
    if (featureName == "sparse_ids") {
        return ConvertSparseMatrixToTensors(data.SparseIds);
    }
    if (featureName == "dense") {
        return {Convert2DimTableToTensor(data.Dense)};
    }
    if (featureName == "dense_seq_ids_apply") {
        return {Convert3DimTableToTensor(data.DenseSeqIds)};
    }
    if (featureName == "dense_seq") {
        return {Convert3DimTableToTensor(data.DenseSeq)};
    }
    if (featureName == "word_ids") {
        return {Convert2DimTableToTensor(data.WordIds)};
    }
    if (featureName == "num_words") {
        return {Convert1DimTableToTensor(data.NumWords)};
    }
    if (featureName == "trigram_batch_map") {
        return {Convert1DimTableToTensor(data.TrigramBatchMap)};
    }
    if (featureName == "trigram_ids") {
        return ConvertSparseMatrixToTensors(data.TrigramIds);
    }
    Y_ENSURE(false, "Unknown feature '" << featureName << "' in encoder model description.");
}

void TEncoder::SaveModelParameters(const TString& modelDirName) const {
    const auto modelDescriptionFileName = GetModelDescriptionFileName(modelDirName);

    TFileOutput modelDescriptionFile(modelDescriptionFileName);
    ModelDescription.Save(&modelDescriptionFile);
}

} // NVins

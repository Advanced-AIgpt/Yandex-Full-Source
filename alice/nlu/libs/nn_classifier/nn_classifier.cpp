#include "nn_classifier.h"

#include <util/stream/file.h>
#include <util/generic/algorithm.h>

using namespace NVins;
using namespace NNeuralNet;
using namespace tensorflow;

namespace {

TString GetModelDescriptionFileName(const TString& modelDirName) {
    return modelDirName + "/model_description";
}

} // anonymous

namespace NVins {

void SaveNnClassifierModelDescription(
    const TString& modelDirName,
    const TNnClassifierModelDescription& modelDescription
) {
    TFileOutput outputFile(GetModelDescriptionFileName(modelDirName));
    modelDescription.Save(&outputFile);
}

TNnClassifier::TNnClassifier(IInputStream* protobufModel, IInputStream* modelDescription)
    : TTfNnModel(protobufModel)
{
    InitializeFromDescription(modelDescription);
}

TNnClassifier::TNnClassifier(const TString& modelDirName)
    : TTfNnModel(modelDirName)
{
    TFileInput modelDescriptionFile(GetModelDescriptionFileName(modelDirName));
    InitializeFromDescription(&modelDescriptionFile);
}

void TNnClassifier::InitializeFromDescription(IInputStream* modelDescription) {
    ModelDescription.Load(modelDescription);

    TVector<TString> inputNodes(ModelDescription.LearningSwitchNodes);
    inputNodes.push_back(ModelDescription.InputNode);

    SetInputNodes(std::move(inputNodes));
    SetOutputNodes({ModelDescription.OutputNode});
}

TVector<size_t> TNnClassifier::PredictFromProba(const TVector<TVector<double>>& probas) const {
    TVector<size_t> result;
    result.reserve(probas.size());
    for (const auto& proba : probas) {
        result.push_back(MaxElement(proba.begin(), proba.end()) - proba.begin());
    }

    return result;
}

TVector<size_t> TNnClassifier::PredictForBatch(Tensor& batch) const {
    const auto probas = PredictProbaForBatch(batch);
    return PredictFromProba(probas);
}

TVector<TVector<double>> TNnClassifier::PredictProbaForBatch(Tensor& batch) const {
    Y_ENSURE(IsSessionEstablished(), "Session is not established.");

    TTensorList inputs;
    const size_t switchNodesNum = ModelDescription.LearningSwitchNodes.size();
    for (size_t switchNodeIdx = 0; switchNodeIdx < switchNodesNum; ++switchNodeIdx) {
        Tensor switchTensor(DT_BOOL, TensorShape({}));
        switchTensor.scalar<bool>()() = false;
        inputs.push_back(switchTensor);
    }
    inputs.push_back(batch);

    auto worker = CreateWorker();
    const auto modelOutput = worker->Process(inputs);
    Y_ENSURE(!modelOutput.empty(), "Model did not output anything.");
    const auto& resultTensor = modelOutput[0];
    Y_ENSURE(resultTensor.dims() == 2, "Output tensor has improper number of dims.");
    const auto& resultData = resultTensor.tensor<float, 2>();

    const size_t samplesNum = resultData.dimension(0);
    TVector<TVector<double>> result(samplesNum, TVector<double>());
    for (size_t sampleIdx = 0; sampleIdx < samplesNum; ++sampleIdx) {
        const size_t classesNum = resultData.dimension(1);
        result[sampleIdx].reserve(classesNum);
        for (size_t classIdx = 0; classIdx < classesNum; ++classIdx) {
            result[sampleIdx].push_back(resultData(sampleIdx, classIdx));
        }
    }

    return result;
}

void TNnClassifier::SaveModelParameters(const TString& modelDirName) const {
    TFileOutput modelDescriptionFile(GetModelDescriptionFileName(modelDirName));
    ModelDescription.Save(&modelDescriptionFile);
}

} // NVins

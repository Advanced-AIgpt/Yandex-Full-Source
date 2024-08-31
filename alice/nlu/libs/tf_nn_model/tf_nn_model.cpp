#include "tf_nn_model.h"

#include <util/stream/file.h>

using namespace NVins;
using namespace NNeuralNet;

namespace {

TString GetModelFileName(const TString& modelDirName, ETfModelFormat modelFormat) {
    return modelDirName + "/model." +
        (modelFormat == ETfModelFormat::Protobuf ? "pb" : "mmap");
}

} //anonymous

namespace NVins {

TTfNnModel::TTfNnModel(IInputStream* protobufModel)
    : ModelFormat(ETfModelFormat::Protobuf)
    , SessionEstablished(false)
{
    GraphProcessor.Reset(TTfGraphProcessorBase::New(*protobufModel));
}

TTfNnModel::TTfNnModel(const TString& memmappedModelDirPath)
    : ModelFormat(ETfModelFormat::Memmapped)
    , SessionEstablished(false)
{
    const TString modelFileName = GetModelFileName(memmappedModelDirPath, ModelFormat);
    GraphProcessor.Reset(TTfGraphProcessorBase::New(modelFileName));
    MemmappedGraph = TBlob::PrechargedFromFile(modelFileName);
}

void TTfNnModel::Save(const TString& modelDirName) const {
    Y_ENSURE(ModelFormat == ETfModelFormat::Memmapped, "Graph was initialized from protobuf, so saving is not supported.");

    TFileOutput modelFile(GetModelFileName(modelDirName, ModelFormat));
    modelFile.Write(MemmappedGraph.Data(), MemmappedGraph.Size());

    SaveModelParameters(modelDirName);
}

void TTfNnModel::EstablishSessionIfNotYet(const TSessionConfig& config) {
    if (!SessionEstablished) {
        EstablishSession(config);
    }
}

bool TTfNnModel::IsSessionEstablished() const {
    return SessionEstablished;
}

TTfWorkerPtr TTfNnModel::CreateWorker() const {
    Y_ENSURE(!InputNodes.empty());
    Y_ENSURE(!OutputNodes.empty());

    return Session->MakeWorker(InputNodes, OutputNodes);
}

void TTfNnModel::EstablishSession(const TSessionConfig& config) {
    auto sessionOptions = GraphProcessor->BuildCommonSessionOptions(
        config.NumInterOpThreads,
        config.NumIntraOpThreads,
        TTfSession::CUDA_NO_DEVICE
    );

    sessionOptions.config.set_use_per_session_threads(config.UsePerSessionThreads);
    Session.Reset(GraphProcessor->MakeSession(sessionOptions));

    SessionEstablished = true;
}

} // NVins

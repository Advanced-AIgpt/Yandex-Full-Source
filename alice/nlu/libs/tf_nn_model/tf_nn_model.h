#pragma once

#include <library/cpp/tf/graph_processor_base/graph_processor_base.h>

#include <util/generic/string.h>
#include <util/memory/blob.h>

namespace NVins {

enum class ETfModelFormat {
    Protobuf,
    Memmapped
};

struct TSessionConfig {
    size_t NumInterOpThreads = 0;
    size_t NumIntraOpThreads = 1;
    bool UsePerSessionThreads = false;
};

class TTfNnModel {
public:
    explicit TTfNnModel(IInputStream* protobufModel);
    explicit TTfNnModel(const TString& memmapedModelDirPath);

    virtual ~TTfNnModel() = default;

    void Save(const TString& modelDirName) const;

    void EstablishSessionIfNotYet(const TSessionConfig& config = TSessionConfig());
    bool IsSessionEstablished() const;

protected:
    void SetInputNodes(TVector<TString>&& inputNodes) {
        InputNodes = std::move(inputNodes);
    }

    void SetOutputNodes(TVector<TString>&& outputNodes) {
        OutputNodes = std::move(outputNodes);
    }

    NNeuralNet::TTfWorkerPtr CreateWorker() const;

    virtual void SaveModelParameters(const TString& modelDirName) const = 0;

private:
    void EstablishSession(const TSessionConfig& config);

private:
    ETfModelFormat ModelFormat;
    TBlob MemmappedGraph;

    NNeuralNet::TTfSessionPtr Session;
    bool SessionEstablished;

    NNeuralNet::TTfGraphProcessorBasePtr GraphProcessor;

    TVector<TString> InputNodes;
    TVector<TString> OutputNodes;
};

} // NVins

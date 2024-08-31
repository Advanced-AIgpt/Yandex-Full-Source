#pragma once

#include <alice/nlu/libs/sample_features/sample_features.h>
#include <alice/nlu/libs/tf_nn_model/tf_nn_model.h>

#include <library/cpp/tf/graph_processor_base/graph_processor_base.h>
#include <contrib/libs/tf/tensorflow/core/lib/core/status.h>

#include <util/generic/hash.h>
#include <util/generic/maybe.h>
#include <util/memory/blob.h>

namespace NVins {

enum class EModelFormat {
    Protobuf,
    Memmapped
};

struct TModelDescription {
    TVector<TString> UsedDenseFeatures;
    TVector<TString> UsedSparseFeatures;
    size_t DecoderDimension = 0;
    size_t NumLabels = 0;
    THashMap<TString, TString> InputsMapping;
    THashMap<TString, TString> OutputsMapping;
    THashMap<TString, THashMap<TString, size_t>> FeaturesMapping;

    Y_SAVELOAD_DEFINE(
        UsedDenseFeatures,
        UsedSparseFeatures,
        DecoderDimension,
        NumLabels,
        InputsMapping,
        OutputsMapping,
        FeaturesMapping
    );
};

void SaveModelDescription(const TString& modelDirName, const TModelDescription& modelDescription);

struct TPrediction {
    TVector<TString> Tags;
    TVector<double> Probabilities;
    double FullProbability = 0.;
    double ClassProbability = 1.;
    bool IsFromThisClass = true;
};

class TRnnTaggerBase {
public:
    explicit TRnnTaggerBase(const TString& modelDirName);
    TRnnTaggerBase(IInputStream* protobufGraph, IInputStream* modelDescription);
    virtual ~TRnnTaggerBase() = default;

    bool UsesSparseFeature(const TString& featureName) const;
    bool UsesDenseFeature(const TString& featureName) const;

    void Save(const TString& modelDirName) const;

    bool IsSessionEstablished() const;
    void EstablishSession(const TSessionConfig& config = TSessionConfig());

    static bool CanBeLoadedFrom(const TString& modelDirName);

public:
    static const TString PROTOBUF_MODEL_FILE;
    static const TString MEMMAPPED_MODEL_FILE;
    static const TString MODEL_DESCRIPTION_FILE;

protected:
    NNeuralNet::TTensorList ConvertDataToEncoderFeed(const TSampleFeatures& data, ssize_t tokenCount) const;

    const TModelDescription& GetModelDescription() const {
        return ModelDescription;
    }

    const TVector<TString>& GetLabelReverseMapping() const {
        return LabelReverseMapping;
    }

    const NNeuralNet::TTfSessionPtr GetSession() const {
        return Session;
    }

private:
    void InitModel();
    void InitLabelReverseMapping();
    void InitSparseFeatureSizes();

    tensorflow::Tensor ConvertWordDataToEncoderFeed(const TSampleFeatures& data, ssize_t tokenCount) const;
    tensorflow::Tensor ConvertPostagDataToEncoderFeed(const TSampleFeatures& data, ssize_t tokenCount) const;
    tensorflow::Tensor ConvertNerDataToEncoderFeed(
        const TSampleFeatures& data,
        ssize_t tokenCount,
        const TString& featureName
    ) const;

private:
    TMaybe<TBlob> MemmappedGraph;
    bool SessionEstablished = false;

    NNeuralNet::TTfGraphProcessorBasePtr GraphProcessor;
    NNeuralNet::TTfSessionPtr Session;

    TModelDescription ModelDescription;
    TVector<TString> LabelReverseMapping;
    THashMap<TString, ssize_t> SparseFeatureSizes;
};

class TRnnTagger : public TRnnTaggerBase {
public:
    explicit TRnnTagger(const TString& modelDirName);
    TRnnTagger(IInputStream* protobufGraph, IInputStream* modelDescription);

    TVector<TVector<TPrediction>> PredictTopForBatch(
        const TVector<TSampleFeatures>& data,
        size_t topSize,
        size_t beamWidth
    ) const;

    virtual TVector<TPrediction> PredictTop(const TSampleFeatures& data, size_t topSize, size_t beamWidth) const;

protected:
    void InitSessionNodes(const TVector<TString>& encoderOutputs);

    NNeuralNet::TTfWorkerPtr CreateEncoder() const;
    NNeuralNet::TTfWorkerPtr CreateDecoder() const;

private:
    TVector<TString> EncoderInputs;
    TVector<TString> EncoderOutputs;

    TVector<TString> DecoderInputs;
    TVector<TString> DecoderOutputs;
};

class TClassifyingRnnTagger : public TRnnTagger {
public:
    explicit TClassifyingRnnTagger(const TString& modelDirName);
    TClassifyingRnnTagger(IInputStream* protobufGraph, IInputStream* modelDescription);

    TVector<TPrediction> PredictTop(const TSampleFeatures& data, size_t topSize, size_t beamWidth) const override;
};

// Uses gready decoding instead of beam search
class TClassifyingSimpleRnnTagger : public TRnnTaggerBase {
public:
    explicit TClassifyingSimpleRnnTagger(const TString& modelDirName);
    TClassifyingSimpleRnnTagger(IInputStream* protobufGraph, IInputStream* modelDescription);

    TPrediction Predict(const TSampleFeatures& data) const;

private:
    void InitSessionNodes();

    NNeuralNet::TTfWorkerPtr CreateWorker() const;

private:
    TVector<TString> Inputs;
    TVector<TString> Outputs;
};

} // namespace NVins

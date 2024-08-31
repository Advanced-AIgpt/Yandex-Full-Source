#pragma once

#include <alice/nlu/libs/tf_nn_model/tf_nn_model.h>

#include <contrib/libs/tf/tensorflow/core/platform/default/integral_types.h>

#include <util/generic/hash.h>

namespace NVins {

struct TEncoderDescription {
    // value is {inputNode} for dense and {indicesNode, valuesNode} for sparse
    THashMap<TString, TVector<TString>> InputsMapping;

    TString Output;

    Y_SAVELOAD_DEFINE(
        InputsMapping,
        Output
    )
};

struct TSparseMatrix {
    TVector<TVector<long long>> Indices;
    TVector<i32> Values;
};

struct TEncoderInput {
    TVector<i32> Lengths;
    TVector<TVector<float>> Dense;
    TVector<TVector<TVector<float>>> DenseSeqIds;
    TVector<TVector<TVector<float>>> DenseSeq;

    TSparseMatrix SparseSeqIds;
    TSparseMatrix SparseIds;

    TVector<TVector<i32>> WordIds;
    TVector<i32> NumWords;
    TVector<i32> TrigramBatchMap;
    TSparseMatrix TrigramIds;
};

void SaveEncoderDescription(
    const TString& modelDirName,
    const TEncoderDescription& encoderDescription
);

class TEncoder : public TTfNnModel {
public:
    explicit TEncoder(const TString& modelDirName);
    TEncoder(IInputStream* protobufModelStream, const TEncoderDescription& modelDescription);

    TVector<TVector<float>> Encode(const TEncoderInput& data) const;

    bool UsesFeature(const TString& featureName);

private:
    void SaveModelParameters(const TString& modelDirName) const override;

    NNeuralNet::TTensorList GetFeed(const TEncoderInput& data) const;
    NNeuralNet::TTensorList GetFeed(
        const TEncoderInput& data,
        const TString& featureName
    ) const;

    void InitModel();

private:
    TEncoderDescription ModelDescription;
};

};

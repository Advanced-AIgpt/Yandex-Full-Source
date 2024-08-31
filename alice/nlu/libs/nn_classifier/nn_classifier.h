#pragma once

#include <alice/nlu/libs/tf_nn_model/batch_helpers.h>
#include <alice/nlu/libs/tf_nn_model/tf_nn_model.h>

#include <library/cpp/tf/graph_processor_base/graph_processor_base.h>
#include <util/stream/input.h>
#include <util/system/types.h>

namespace NVins {

struct TNnClassifierModelDescription {
    size_t BatchSize;
    size_t PaddingLength;
    TString InputNode;
    TString OutputNode;
    TVector<TString> LearningSwitchNodes;

    Y_SAVELOAD_DEFINE(
        BatchSize,
        PaddingLength,
        InputNode,
        OutputNode,
        LearningSwitchNodes
    );
};

void SaveNnClassifierModelDescription(
    const TString& modelDirName,
    const TNnClassifierModelDescription& modelDescription
);

class TNnClassifier : public TTfNnModel {
public:
    explicit TNnClassifier(IInputStream* protobufModel, IInputStream* modelDescription);
    explicit TNnClassifier(const TString& modelDirName);

    template<typename T>
    TVector<size_t> Predict(const TVector<T>& data) const {
        return PredictFromProba(PredictProba(data));
    }

    template<typename T>
    TVector<TVector<double>> PredictProba(const TVector<T>& data) const {
        auto processor = [&](
            const TVector<T>& data, size_t batchStart, size_t batchEnd
        ) -> TVector<TVector<double>> {
            auto paddedBatch = GetPaddedBatch(
                data, batchStart, batchEnd, ModelDescription.PaddingLength
            );
            return PredictProbaForBatch(paddedBatch);
        };
        return ProcessInPaddedBatches<TVector<double>>(data, ModelDescription.BatchSize, processor);
    }

    TVector<size_t> PredictFromProba(const TVector<TVector<double>>& probas) const;

private:
    void InitializeFromDescription(IInputStream* modelDescription);

    TVector<size_t> PredictForBatch(tensorflow::Tensor& batch) const;
    TVector<TVector<double>> PredictProbaForBatch(tensorflow::Tensor& batch) const;

    void SaveModelParameters(const TString& modelDirName) const override;

protected:
    TNnClassifierModelDescription ModelDescription;
};

};

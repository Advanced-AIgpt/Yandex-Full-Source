#pragma once

#include "dssm_based_binary_classifier.h"

#include <catboost/libs/model/model.h>

#include <util/generic/array_ref.h>
#include <util/generic/ymath.h>

namespace NAlice {

struct TBegginsBinaryClassifier {
    virtual ~TBegginsBinaryClassifier() = default;
    virtual float PredictProbability(const TVector<float>& queryEmbedding) const = 0;
};

class TBegginsTensorflowBinaryClassifier final : public TBegginsBinaryClassifier, public TDssmBasedBinaryClassifier {
public:
    TBegginsTensorflowBinaryClassifier(IInputStream* protobufModelStream, IInputStream* jsonConfigStream)
        : TDssmBasedBinaryClassifier(protobufModelStream, jsonConfigStream)
    {
    }
    TBegginsTensorflowBinaryClassifier(IInputStream* protobufModelStream, const TConfig& config)
        : TDssmBasedBinaryClassifier(protobufModelStream, config)
    {
    }

    float PredictProbability(const TVector<float>& queryEmbedding) const override;
};

class TBegginsCatBoostBinaryClassifier final : public TBegginsBinaryClassifier {
public:
    TBegginsCatBoostBinaryClassifier(IInputStream* cbmModelStream) {
        Classifier.Load(cbmModelStream);
    }

    float PredictProbability(const TVector<float>& queryEmbedding) const override {
        TVector<double> results(1);
        Classifier.Calc(/* floatFeatures= */ MakeConstArrayRef(queryEmbedding), /* catFeatures= */{}, MakeArrayRef(results));
        const auto rawFormulaValue = results[0];
        return rawFormulaValue;
    }

private:
    TFullModel Classifier;
}; 

using TBegginsBinaryClassifierPtr = THolder<TBegginsBinaryClassifier>;

} // namespace NAlice

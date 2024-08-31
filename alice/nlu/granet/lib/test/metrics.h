#pragma once

#include <alice/nlu/granet/lib/sample/markup.h>
#include <util/generic/map.h>
#include <util/stream/output.h>

namespace NGranet {

inline double SafeRatio(double numerator, double denominator, double defaultResult = 1.) {
    return denominator == 0 ? defaultResult : numerator / denominator;
}

// ~~~~ TSampleProcessingInfo ~~~~

struct TSampleProcessingInfo {
    double Weight = 1.;
    TSampleMarkup Expected;
    TSampleMarkup Actual;
    bool CompareSlotsByTop = true;
    double Time = 0; // microseconds
    TString Comment;
};

// ~~~~ TErrorMetrics ~~~~

struct TErrorMetrics {
public:
    double FalsePositive = 0;
    double FalseNegative = 0;
    double TruePositive = 0;
    double TrueNegative = 0;

    // True-positive samples with incorrect tagger result.
    double TaggerMismatch = 0;

public:
    double Total() const {
        return FalsePositive + FalseNegative + TruePositive + TrueNegative;
    }

    double ExpectedPositive() const {
        return TruePositive + FalseNegative;
    }
    double ExpectedNegative() const {
        return TrueNegative + FalsePositive;
    }
    double ResultPositive() const {
        return TruePositive + FalsePositive;
    }
    double ResultNegative() const {
        return TrueNegative + FalseNegative;
    }

    double Precision() const {
        return SafeRatio(TruePositive, ResultPositive());
    }
    double Recall() const {
        return SafeRatio(TruePositive, ExpectedPositive());
    }
    double TaggerAccuracy() const {
        return SafeRatio(TruePositive - TaggerMismatch, ExpectedPositive());
    }
    double ClassifierAccuracy() const {
        return SafeRatio(TruePositive + TrueNegative, Total());
    }
    double FullAccuracy() const {
        return SafeRatio(TruePositive + TrueNegative - TaggerMismatch, Total());
    }

    bool HasAnyError() const {
        return FalsePositive > 0. || FalseNegative > 0. || TaggerMismatch > 0.;
    }

    double& CounterRW(bool isExpectedPositive, bool isResultPositive);

    void AddSample(double weight, bool isExpectedPositive, bool isResultPositive, bool isTaggerError);
    void AddOther(const TErrorMetrics& other);
};

// ~~~~ TIntentMetrics ~~~~

struct TIntentMetrics {
public:
    size_t SampleCount = 0;

    TErrorMetrics ErrorsByCount;
    TErrorMetrics ErrorsByWeight;

    // Time in microseconds
    double TimeSum = 0;
    double TimeMax = 0;

public:
    const TErrorMetrics& Errors(bool byWeight) const;
    bool HasAnyError() const;
    double TimeAvg() const;

    void AddSample(const TSampleProcessingInfo& sample);
    void AddOther(const TIntentMetrics& other);
};

// ~~~~ TBatchMetrics ~~~~

struct TBatchMetrics {
public:
    // Intent name with optional prefix -> metrics.
    TMap<TString, TIntentMetrics> Metrics;

public:
    void AddIntent(const TString& name, const TIntentMetrics& metrics);
    void AddOther(const TBatchMetrics& other);
};

// ~~~~ Report formatting ~~~~

void PrintSelectionReport(const TIntentMetrics& metrics, TStringBuf name, IOutputStream* out,
    const TString& indent = "");
void PrintTestingReport(const TIntentMetrics& metrics, TStringBuf name, bool needTimings,
    IOutputStream* out, const TString& indent = "");

void PrintTestingReport(const TBatchMetrics& metrics, bool byWeight, bool needTimings,
    IOutputStream* out, const TString& indent = "");
void PrintTestingReport(const TBatchMetrics& metrics, IOutputStream* out, const TString& indent = "");

} // namespace NGranet

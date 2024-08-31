#pragma once

#include <util/datetime/base.h>
#include <util/generic/maybe.h>
#include <util/generic/vector.h>
#include <util/memory/blob.h>

#include <memory>

namespace NAlice::NBeggins {

struct TEmbedderOptions {
    // YNMT specific options
    ui32 MaxBatchSize = 128;
    ui32 ExpectedMaxSubBatchesNum = 32;
    // Specifies the number of threads to use for OpenMP/MKL on the CPU backend (will have no effects on other backends)
    ui32 NumThreads = 1;

    TMaybe<ui32> GpuDeviceId;
};

class IEmbedder {
public:
    struct TResult {
        TVector<float> SentenceEmbedding;
        TMaybe<TDuration> ProcessingTime;
        TMaybe<ui32> BatchSize;
    };

public:
    virtual ~IEmbedder() = default;
    virtual TVector<float> GetSentenceEmbedding(const TString& query) const = 0;

    virtual TResult Process(const TString& /* query */) const {
        return {};
    }
};

// Loads embedder from blob, for instance LoadEmbedder(TBlob::FromFileContent("test_models/model.bert.htxt"))
std::unique_ptr<IEmbedder> LoadEmbedder(TBlob blob, const TEmbedderOptions& options);

} // namespace NAlice::NBeggins

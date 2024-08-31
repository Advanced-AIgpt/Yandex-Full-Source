#include "embedder.h"

#include <alice/beggins/internal/bert/embedder.h>
#include <alice/beggins/internal/bert_applier/applier.h>

#include <utility>

namespace NAlice::NBeggins {

namespace {

class TEmbedder final : public IEmbedder {
public:
    explicit TEmbedder(std::unique_ptr<NInternal::TBertEmbedder> embedder)
        : Embedder(std::move(embedder)) {
        Y_ENSURE(Embedder, "Embedder must not be null");
    }

    TVector<float> GetSentenceEmbedding(const TString& query) const final {
        return NInternal::TBertEmbedder::ExtractEmbeddings(Embedder->Process(query));
    }

private:
    std::unique_ptr<NInternal::TBertEmbedder> Embedder;
};

class TGPUEmbedder final : public IEmbedder {
public:
    TGPUEmbedder(std::unique_ptr<NInternal::TBertBatchProcessor> embedder, TIntrusivePtr<ITimeProvider> timeProvider)
        : Embedder(std::move(embedder))
        , TimeProvider(timeProvider) {
        Y_ENSURE(Embedder, "Embedder must not be null");
    }

    TVector<float> GetSentenceEmbedding(const TString& query) const final {
        const auto result = Embedder->Process(query).GetValueSync();
        return NInternal::TBertEmbedder::ExtractEmbeddings(result.Embedding);
    }

    TResult Process(const TString& query) const final {
        const auto result = Embedder->Process(query).GetValueSync();
        return {
            .SentenceEmbedding = NInternal::TBertEmbedder::ExtractEmbeddings(result.Embedding),
            .ProcessingTime = result.ProcessingTime,
            .BatchSize = result.BatchSize
        };
    }

private:
    mutable std::unique_ptr<NInternal::TBertBatchProcessor> Embedder;
    mutable TIntrusivePtr<ITimeProvider> TimeProvider;
};

} // namespace

std::unique_ptr<IEmbedder> LoadEmbedder(TBlob blob, const TEmbedderOptions& options) {
    NBertModels::TTechnicalOpenModelOptions technicalOpenModelOptions;
    technicalOpenModelOptions.NumThreads = options.NumThreads;
    technicalOpenModelOptions.ExpectedMaxSubBatchesNum = options.ExpectedMaxSubBatchesNum;
    technicalOpenModelOptions.MaxBatchSize = options.MaxBatchSize;
    if (!options.GpuDeviceId.Defined()) {
        Cerr << "Using CPU embedder" << Endl;
        return std::make_unique<TEmbedder>(NInternal::LoadEmbedder(blob, technicalOpenModelOptions));
    }
    Cerr << "Using GPU embedder" << Endl;
    technicalOpenModelOptions.UseGPU = true;
    const ui32 gpuDeviceId = *options.GpuDeviceId;
    Cerr << "Using GPU DeviceID: " << gpuDeviceId << Endl;
    technicalOpenModelOptions.GpuDeviceIndiciesPool = new NBertModels::TGpuDeviceIndiciesPool({gpuDeviceId});
    auto timeProvider = CreateDefaultTimeProvider();
    return std::make_unique<TGPUEmbedder>(
        NInternal::LoadBertBatchProcessor(blob, *timeProvider, technicalOpenModelOptions), timeProvider);
}

} // namespace NAlice::NBeggins

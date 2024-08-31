#pragma once

// WARNING: it's full copy of search/begemot/rules/bert_inference/batch_processor.{h,cpp}

#include <library/cpp/threading/future/future.h>
#include <library/cpp/time_provider/time_provider.h>

#include <quality/relev_tools/bert_models/lib/model_descr_metadata/result_descr.pb.h>
#include <quality/relev_tools/bert_models/lib/multi_tool.h>

#include <util/generic/array_ref.h>
#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <util/system/event.h>
#include <util/system/spinlock.h>
#include <util/system/thread.h>

#include <atomic>
#include <list>

namespace NAlice::NBeggins::NInternal {

class TBertBatchProcessor {
private:
    struct RequestResult {
        NBertModels::NProto::TExportableDocResult Embedding;
        NBertModels::TPredictsOutput Predicts;
        TMaybe<TDuration> ProcessingTime;
        TMaybe<ui32> BatchSize;
    };

    struct TRequestInfo {
        NThreading::TPromise<RequestResult> Promise;
        TInstant Deadline;
        NBertModels::TDocInBatchId DocId;
    };

    struct TBatch {
        NBertModels::TMetaGraphEvaluationMutableContext BatchContext;
        TVector<TRequestInfo> Requests;

        TBatch(size_t maxBatchSize)
            : BatchContext(maxBatchSize)
            , Requests(Reserve(maxBatchSize)) {
        }

        bool Empty() const noexcept {
            return Requests.empty();
        }

        size_t Size() const noexcept {
            return Requests.size();
        }
    };

private:
    /// an object that holds tokenization model
    TVector<THolder<NBertModels::TMultiTool>> TokenizationModel;
    /// an object that holds evaluation model
    TVector<THolder<NBertModels::TMultiTool>> EvaluationModel;
    ITimeProvider& TimeProvider;
    TManualEvent RunProcessing;
    TThread ProcessingThread;
    std::atomic_bool Shutdown = false;
    const TDuration Timeout = TDuration::MilliSeconds(5);
    TAdaptiveLock RequestsLock;
    TVector<std::list<TBatch>> Batches;
    size_t MaxBatchSize;

public:
    //! Constructs the processor
    /** @param tokenizationModel - The production model to perform preprocessing
        @param evaluationModel - The production model to perform main computation
        @param timeProvider - The time provider
        @param maxBatchSize - The maximum size of a batch to process
        @param accumulationPeriod - The batch accumulation timeout. Process method will try to accumulate enough data
    for the batch during the given time period
        @param devTokenizationModel - The development model to perform preprocessing
        @param devEvaluationModel - The development model to perform main computation
    **/
    TBertBatchProcessor(THolder<NBertModels::TMultiTool> tokenizationModel,
                        THolder<NBertModels::TMultiTool> evaluationModel, ITimeProvider& timeProvider,
                        size_t maxBatchSize = 32, TDuration accumulationPeriod = TDuration::MilliSeconds(5),
                        THolder<NBertModels::TMultiTool> devTokenizationModel = nullptr,
                        THolder<NBertModels::TMultiTool> devEvaluationModel = nullptr);
    ~TBertBatchProcessor();

    //! Starts batch processing of the data.
    /** Would run synchronously if enough data for the complete batch available and forceAsync is false
        Otherwise will schedule the data for an asynchronous processing
        @param data - The data to process
        @param forceAsync - An indicator of processing in a different thread
        @param isDev - An indicator of processing with the development model
    */
    NThreading::TFuture<RequestResult> Process(NBertModels::TTextualDocumentRepresentation&& data,
                                               bool forceAsync = false, bool isDev = false);

    NThreading::TFuture<RequestResult> Process(const TStringBuf query, bool forceAsync = false, bool isDev = false);

    //! Processes the data and all pending data immediately
    /** This could be inefficient since there could be not enough data for the complete batch
        @param data - The data to process
        @param isDev - An indicator of processing with the development model
    */
    RequestResult ProcessImmediatly(NBertModels::TTextualDocumentRepresentation&& data, bool isDev = false);

    //! Cancels all requests those are not being processed at the moment
    void Cancel();

    //! Returns the maximum batch size
    inline size_t GetMaxBatchSize() const noexcept {
        return MaxBatchSize;
    }

    //! Returns the model version
    /**
        @param isDev - An indicator of the development model
    */
    inline TString GetModelVersion(bool isDev = false) const noexcept {
        return EvaluationModel[isDev]->GetConfig().GetStorageUniqId();
    }

private:
    TBertBatchProcessor(TBertBatchProcessor const&) = delete;
    TBertBatchProcessor& operator=(TBertBatchProcessor const&) = delete;

    void ProcessingProc();
    void TryProcess(TGuard<decltype(RequestsLock)>&& guard, bool forceAsync);
    void Process(TGuard<decltype(RequestsLock)>&& guard, bool isDev);
    NBertModels::NProto::TExportableDocResult Preprocess(NBertModels::TTextualDocumentRepresentation&& data,
                                                         bool isDev);
    NThreading::TFuture<RequestResult> EnqueueRequest(NBertModels::NProto::TExportableDocResult&& data, bool isDev);
    void CreateNewBatch(bool isDev);
};

std::unique_ptr<TBertBatchProcessor>
LoadBertBatchProcessor(TBlob file, ITimeProvider& timeProvider,
                       const NBertModels::TTechnicalOpenModelOptions& options = {});

} // namespace NAlice::NBeggins::NInternal

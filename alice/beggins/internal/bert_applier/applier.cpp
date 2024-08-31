#include "applier.h"

#include <kernel/search_query/cnorm.h>

#include <library/cpp/float16/float16.h>
#include <library/cpp/threading/cancellation/operation_cancelled_exception.h>

#include <quality/relev_tools/bert_models/lib/one_file_package.h>

#include <util/generic/xrange.h>
#include <util/generic/yexception.h>
#include <util/string/cast.h>
#include <util/system/guard.h>

#include <algorithm>
#include <exception>

using namespace NThreading;

namespace NAlice::NBeggins::NInternal {

TBertBatchProcessor::TBertBatchProcessor(THolder<NBertModels::TMultiTool> tokenizationModel,
                                         THolder<NBertModels::TMultiTool> evaluationModel, ITimeProvider& timeProvider,
                                         size_t maxBatchSize, TDuration accumulationPeriod,
                                         THolder<NBertModels::TMultiTool> devTokenizationModel,
                                         THolder<NBertModels::TMultiTool> devEvaluationModel)
    : TimeProvider(timeProvider)
    , RunProcessing()
    , ProcessingThread([this] { this->ProcessingProc(); })
    , Shutdown(false)
    , Timeout(accumulationPeriod)
    , RequestsLock()
    , Batches(2)
    , MaxBatchSize(maxBatchSize) {
    TokenizationModel.push_back(std::move(tokenizationModel));
    TokenizationModel.push_back(std::move(devTokenizationModel));
    EvaluationModel.push_back(std::move(evaluationModel));
    EvaluationModel.push_back(std::move(devEvaluationModel));
    CreateNewBatch(true);
    CreateNewBatch(false);
    ProcessingThread.Start();
}

TBertBatchProcessor::~TBertBatchProcessor() {
    {
        bool expected = false;
        if (!Shutdown.compare_exchange_strong(expected, true)) {
            return;
        }
        RunProcessing.Signal();
    }
    ProcessingThread.Join();
}

TFuture<TBertBatchProcessor::RequestResult>
TBertBatchProcessor::Process(NBertModels::TTextualDocumentRepresentation&& data, bool forceAsync, bool isDev) {
    if (!(TokenizationModel[isDev] && EvaluationModel[isDev]))
        throw yexception() << (isDev ? "Dev" : "Prod") << " model is not initialized";
    NBertModels::NProto::TExportableDocResult preprocessedData = Preprocess(std::move(data), isDev);
    TGuard g(RequestsLock);
    auto result = EnqueueRequest(std::move(preprocessedData), isDev);
    TryProcess(std::move(g), forceAsync);
    return result;
}

TFuture<TBertBatchProcessor::RequestResult> TBertBatchProcessor::Process(const TStringBuf query, bool forceAsync,
                                                                         bool isDev) {
    return Process(
        NBertModels::TTextualDocumentRepresentation{
            {"QueryBertNormed", NCnorm::BNorm(query)},
            {"BaseRegionNamesRus_BetaBertNormed", "россия"},
        },
        forceAsync, isDev);
}

TBertBatchProcessor::RequestResult
TBertBatchProcessor::ProcessImmediatly(NBertModels::TTextualDocumentRepresentation&& data, bool isDev) {
    if (!(TokenizationModel[isDev] && EvaluationModel[isDev]))
        throw yexception() << (isDev ? "Dev" : "Prod") << " model is not initialized";
    NBertModels::NProto::TExportableDocResult preprocessedData = Preprocess(std::move(data), isDev);
    TGuard g(RequestsLock);
    auto future = EnqueueRequest(std::move(preprocessedData), isDev);
    Process(std::move(g), isDev);
    return future.ExtractValueSync();
}

void TBertBatchProcessor::Cancel() {
    TGuard g(RequestsLock);
    bool allEmpty = true;
    for (auto& batchList : Batches)
        if (!batchList.front().Empty()) {
            allEmpty = false;
            break;
        }
    if (allEmpty)
        return;

    auto e = std::make_exception_ptr(TOperationCancelledException());
    for (auto& batchList : Batches) {
        for (auto& b : batchList)
            for (auto& r : b.Requests) {
                r.Promise.SetException(e);
            }
        batchList.clear();
    }
    CreateNewBatch(false);
    CreateNewBatch(true);
}

void TBertBatchProcessor::ProcessingProc() {
    TThread::SetCurrentThreadName("bert_batch");
    TDuration timeout = Timeout;
    while (!Shutdown) {
        RunProcessing.WaitT(timeout);
        RunProcessing.Reset();
        while (!Shutdown) {
            TGuard g(RequestsLock);
            // wait if there are no data to process
            bool isNothingToProcess = true;
            for (auto& b : Batches) {
                Y_VERIFY(!b.empty());
                if (!b.front().Empty()) {
                    isNothingToProcess = false;
                    break;
                }
            }
            if (isNothingToProcess) {
                timeout = Timeout;
                break;
            }

            // choose the batch with the earliest deadline
            int modelIdx = 0;
            TDuration nextTimeout = Timeout;
            bool isProcess = false;
            auto const now = TimeProvider.Now();
            for (auto i = 0U; i < Batches.size(); ++i) {
                auto const& front = Batches[i].front();
                if (front.Empty())
                    continue;
                if (front.Size() >= GetMaxBatchSize() || front.Requests.front().Deadline <= now) {
                    if (!isProcess ||
                        Batches[modelIdx].front().Requests.front().Deadline > front.Requests.front().Deadline)
                        modelIdx = i;
                    isProcess = true;
                } else {
                    nextTimeout = Min(nextTimeout, front.Requests.front().Deadline - now);
                }
            }
            if (!isProcess) {
                timeout = nextTimeout;
                break;
            }
            Process(std::move(g), modelIdx);
        }
    }
}

void TBertBatchProcessor::TryProcess(TGuard<decltype(RequestsLock)>&& guard, bool forceAsync) {
    // The guard should be acquired
    auto const& devBatch = Batches[true].front();
    auto const& prodBatch = Batches[false].front();
    auto const now = TimeProvider.Now();
    bool isWaitingDev =
        devBatch.Size() < GetMaxBatchSize() && (devBatch.Empty() || devBatch.Requests.front().Deadline > now);
    bool isWaitingProd =
        prodBatch.Size() < GetMaxBatchSize() && (prodBatch.Empty() || prodBatch.Requests.front().Deadline > now);
    if (isWaitingDev && isWaitingProd) {
        return;
    }
    if (!forceAsync) {
        bool isDev;
        // data for both models are ready
        if (!isWaitingDev && !isWaitingProd) {
            // dev and prod batches are not empty.
            // is the dev batch has earlier deadline?
            isDev = devBatch.Requests.front().Deadline < prodBatch.Requests.front().Deadline;
        } else
            // if isWaitingDev then a batch for the prodion model is ready (isDev=false)
            // if !isWaitingDev then a batch for the development model is ready (isDev=true)
            isDev = !isWaitingDev;
        Process(std::move(guard), isDev);
    } else {
        guard.Release();
        RunProcessing.Signal();
    }
}

void TBertBatchProcessor::Process(TGuard<decltype(RequestsLock)>&& guard, bool isDev) {
    // The guard should be acquired
    auto const now = TimeProvider.Now();
    std::list<TBatch> batches;
    auto const sliceEnd =
        std::find_if(std::begin(Batches[isDev]), std::end(Batches[isDev]), [now, this](TBatch const& b) {
            return b.Empty() || (b.Size() < GetMaxBatchSize() && b.Requests.front().Deadline > now);
        });
    batches.splice(std::end(batches), Batches[isDev], std::begin(Batches[isDev]), sliceEnd);
    if (Batches[isDev].empty()) {
        CreateNewBatch(isDev);
    }

    guard.Release();

    for (auto& batch : batches) {
        // process data
        auto start_time = TimeProvider.Now();
        EvaluationModel[isDev]->DoRunOverContext(batch.BatchContext);
        auto processingTime = TimeProvider.Now() - start_time;

        bool writeLog = true;
        for (auto r : batch.Requests) {
            RequestResult res;
            // res.Predicts = EvaluationModel[isDev]->ExtractPredict(r.DocId, batch.BatchContext);
            res.Embedding = EvaluationModel[isDev]->ExtractExportableDocResult(r.DocId, batch.BatchContext);
            if (writeLog) {
                writeLog = false;
                res.BatchSize = batch.Size();
                res.ProcessingTime = processingTime;
            }
            r.Promise.SetValue(res);
        }
    }
}

NBertModels::NProto::TExportableDocResult
TBertBatchProcessor::Preprocess(NBertModels::TTextualDocumentRepresentation&& data, bool isDev) {
    NBertModels::TMetaGraphEvaluationMutableContext context(1);
    auto docId = context.AddDocumentByTextInputs(std::move(data));
    TokenizationModel[isDev]->DoRunOverContext(context);
    return TokenizationModel[isDev]->ExtractExportableDocResult(docId, context);
}

TFuture<TBertBatchProcessor::RequestResult>
TBertBatchProcessor::EnqueueRequest(NBertModels::NProto::TExportableDocResult&& data, bool isDev) {
    // Should be called under the RequestsLock
    if (Batches[isDev].back().Size() == GetMaxBatchSize()) {
        CreateNewBatch(isDev);
    }
    auto const deadline = TimeProvider.Now() + Timeout;
    auto promise = NewPromise<RequestResult>();
    auto result = promise.GetFuture();
    auto& batch = Batches[isDev].back();
    auto docId = batch.BatchContext.AddDocument();
    EvaluationModel[isDev]->UpdateDocumentInContext(docId, std::move(data), batch.BatchContext);
    batch.Requests.push_back({std::move(promise), deadline, docId});
    return result;
}

void TBertBatchProcessor::CreateNewBatch(bool isDev) {
    // Should be called under the RequestsLock (unless called from the .ctor)
    Batches[isDev].emplace_back(GetMaxBatchSize());
}

std::unique_ptr<TBertBatchProcessor> LoadBertBatchProcessor(TBlob file, ITimeProvider& timeProvider,
                                                            const NBertModels::TTechnicalOpenModelOptions& options) {
    auto evaluationModel = NBertModels::ReadPackedModel(file, options,
                                                        /* layout= */ "BegemotPartWithoutTokenization");
    auto tokenizationModel =
        evaluationModel->ReOpen(evaluationModel->GetConfig(), /* layout= */ "BegemotPartTokenization");
    return std::make_unique<TBertBatchProcessor>(std::move(tokenizationModel), std::move(evaluationModel),
                                                 timeProvider, options.ExpectedMaxSubBatchesNum,
                                                 TDuration::MilliSeconds(1));
}

} // namespace NAlice::NBeggins::NInternal

#include <alice/cachalot/library/golovan.h>

#include <util/generic/singleton.h>


namespace {

    const NUnistat::TIntervals HIGH_RESOLUTION_COUNTER_VALUES({
        0, 0.001, 0.0025, 0.005, 0.0075, 0.010, 0.0125, 0.015, 0.0175, 0.020,
        0.025, 0.030, 0.035, 0.040, 0.045, 0.050, 0.06, 0.07, 0.08, 0.09, 0.1,
        0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0, 2.0, 3.0, 5.0, 7.5, 10.0
    });

    const NUnistat::TIntervals SIZE_COUNTER_VALUES({
        0, 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384,
        32768, 65536, 131072, 262144, 524288, 1048576, 2097152, 4194304
    });

    ::NUnistat::IHolePtr CreateCounter(const TString& name, const TString& suffix = "summ") {
        return TUnistat::Instance().DrillFloatHole(
            name,
            suffix,
            ::NUnistat::TPriority(0),
            ::NUnistat::TStartValue(0)
        );
    }

    ::NUnistat::IHolePtr CreateAbsCounter(const TString& name, const TString& suffix = "ammx") {
        return TUnistat::Instance().DrillFloatHole(
            name,
            suffix,
            ::NUnistat::TPriority(0),
            ::NUnistat::TStartValue(0),
            EAggregationType::LastValue
        );
    }

    ::NUnistat::IHolePtr CreateHgramCounter(const TString& name, const NUnistat::TIntervals& intervals) {
        return TUnistat::Instance().DrillHistogramHole(
            name,
            "hgram",
            ::NUnistat::TPriority(0),
            intervals
        );
    }

    void Increment(NUnistat::IHolePtr& ptr) {
        ptr->PushSignal(1.0);
    }

    void Measure(NUnistat::IHolePtr& hgram, const TInstant startTime) {
        const TDuration dur = TInstant::Now() - startTime;
        hgram->PushSignal(dur.SecondsFloat());
    }

}  // anonymous namespace


namespace NCachalot {


TEventMetricBase::TEventMetricBase(const TString& eventName)
    : EventCounter(CreateCounter(eventName))
{
}

TEventMetricBase::TEventMetricBase(NUnistat::IHolePtr eventCounter)
    : EventCounter(std::move(eventCounter))
{
}

void TEventMetricBase::OnEvent(const TInstant /*startTime*/) {
    Increment(EventCounter);
}


TEventMetric::TEventMetric(const TString& eventName)
    : TEventMetricBase(eventName)
    , DurationConter(CreateHgramCounter(eventName + "_time", HIGH_RESOLUTION_COUNTER_VALUES))
{
}

TEventMetric::TEventMetric(NUnistat::IHolePtr eventCounter, NUnistat::IHolePtr durationConter)
    : TEventMetricBase(std::move(eventCounter))
    , DurationConter(std::move(durationConter))
{
}

void TEventMetric::OnEvent(const TInstant startTime) {
    TEventMetricBase::OnEvent(startTime);
    Measure(DurationConter, startTime);
}


TYdbMetrics::TYdbMetrics(const TString& operationName)
    : ExecuteError(operationName + "_execute_error")
    , NotFound(operationName + "_not_found")
    , Ok(operationName + "_ok")
    , OperationStarted(operationName + "_operation_started")
    , RetryError(operationName + "_retry_error")
    , AssertionError(operationName + "_assertion_error")
    , ExecuteResponse(operationName + "_execute_response")
    , ExecuteException(operationName + "_execute_exception")
{
}

void TYdbMetrics::OnExecuteError(const TInstant startTime) {
    ExecuteError.OnEvent(startTime);
}

void TYdbMetrics::OnNotFound(const TInstant startTime) {
    NotFound.OnEvent(startTime);
}

void TYdbMetrics::OnOk(const TInstant startTime) {
    Ok.OnEvent(startTime);
}

void TYdbMetrics::OnOperationStarted(const TInstant startTime) {
    OperationStarted.OnEvent(startTime);
}

void TYdbMetrics::OnRetryError(const TInstant startTime) {
    RetryError.OnEvent(startTime);
}

void TYdbMetrics::OnAssertionError(const TInstant startTime) {
    AssertionError.OnEvent(startTime);
}

void TYdbMetrics::OnExecuteResponse(const TInstant startTime) {
    ExecuteResponse.OnEvent(startTime);
}

void TYdbMetrics::OnExecuteException(const TInstant startTime) {
    ExecuteException.OnEvent(startTime);
}


TRedisMetrics::TRedisMetrics(const TString& operationName)
    : Error(operationName + "_error")
    , Ok(operationName + "_ok")
{
}

void TRedisMetrics::OnError(const TInstant startTime) {
    Error.OnEvent(startTime);
}

void TRedisMetrics::OnOk(const TInstant startTime) {
    Ok.OnEvent(startTime);
}


TRedisGetMetrics::TRedisGetMetrics(const TString& operationName)
    : TRedisMetrics(operationName)
    , CacheHit(operationName + "_cache_hit")
    , CacheMiss(operationName + "_cache_miss")
{
}

void TRedisGetMetrics::OnCacheHit(const TInstant startTime) {
    TRedisMetrics::OnOk(startTime);
    CacheHit.OnEvent(startTime);
}

void TRedisGetMetrics::OnCacheMiss(const TInstant startTime) {
    TRedisMetrics::OnOk(startTime);
    CacheMiss.OnEvent(startTime);
}


TInmemoryStorageOperationMetricsBase::TInmemoryStorageOperationMetricsBase(const TString& prefix)
    : Ok(prefix + "_ok")
    , Failed(prefix + "_failed")
    , QueueFullErrorCounter(CreateCounter(prefix + "_queue_full_error"))
{
}

void TInmemoryStorageOperationMetricsBase::OnOk(const TInstant startTime) {
    Ok.OnEvent(startTime);
}

void TInmemoryStorageOperationMetricsBase::OnFailed(const TInstant startTime) {
    Failed.OnEvent(startTime);
}

void TInmemoryStorageOperationMetricsBase::OnQueueFull() {
    Increment(QueueFullErrorCounter);
}


TInmemoryStorageLoadMetrics::TInmemoryStorageLoadMetrics(const TString& prefix)
    : TInmemoryStorageOperationMetricsBase(prefix + "_load")
    , NotFound(prefix + "_load_not_found")
{
}

void TInmemoryStorageLoadMetrics::OnNotFound(const TInstant startTime) {
    NotFound.OnEvent(startTime);
}


TInmemoryStorageStoreMetrics::TInmemoryStorageStoreMetrics(const TString& prefix)
    : TInmemoryStorageOperationMetricsBase(prefix + "_store")
{
}


TInmemoryStorageRemoveMetrics::TInmemoryStorageRemoveMetrics(const TString& prefix)
    : TInmemoryStorageOperationMetricsBase(prefix + "_remove")
    , NotFound(prefix + "_remove_not_found")
{
}

void TInmemoryStorageRemoveMetrics::OnNotFound(const TInstant startTime) {
    NotFound.OnEvent(startTime);
}


TInmemoryStorageMetrics::TInmemoryStorageMetrics(const TString& handleName)
    : Get(handleName + "_imdb")
    , Set(handleName + "_imdb")
    , Delete(handleName + "_imdb")
    , MemoryUsage(
        TUnistat::Instance().DrillFloatHole(
            handleName + "_imdb_memory_usage_bytes",
            "ammx",
            NUnistat::TPriority(0),
            NUnistat::TStartValue(0),
            EAggregationType::LastValue
        )
    )
    , NumberOfItems(
        TUnistat::Instance().DrillFloatHole(
            handleName + "_imdb_number_of_items",
            "ammx",
            NUnistat::TPriority(0),
            NUnistat::TStartValue(0),
            EAggregationType::LastValue
        )
    )
{
}

TInmemoryStorageMetricsContainer::TInmemoryStorageMetricsContainer(TString handleName)
    : HandleName(std::move(handleName))
{
}

TInmemoryStorageMetrics& TInmemoryStorageMetricsContainer::operator[](const TString& storageTag) {
    if (auto it = StorageTag2Metrics.find(storageTag); it != StorageTag2Metrics.end()) {
        return it->second;
    } else {
        // All values are initialized during service startup, so race-condition is not possible
        StorageTag2Metrics.emplace(storageTag, TInmemoryStorageMetrics(HandleName + "_" + storageTag));
        return this->operator[](storageTag);
    }
}

void TInmemoryStorageMetrics::SetMemoryUsage(uint64_t memoryUsageBytes) {
    MemoryUsage->PushSignal(memoryUsageBytes);
}

void TInmemoryStorageMetrics::SetNumberOfItems(uint64_t numberOfItems) {
    NumberOfItems->PushSignal(numberOfItems);
}

TRequestMetrics::TRequestMetrics(const TString& handleName)
    : InternalErrorCounter(CreateCounter(handleName + "_internal_error"))
    , InvalidRequestCounter(CreateCounter(handleName + "_invalid_request"))
    , QueueFullErrorCounter(CreateCounter(handleName + "_queue_full_error"))
    , RequestsTotalCounter(CreateCounter(handleName + "_requests_total"))
    , FailedCounter(CreateCounter(handleName + "_failed"))
    , TimeoutCounter(CreateCounter(handleName + "_timeout"))
    , UnexpectedCounter(CreateCounter(handleName + "_unexpected"))
    , ErrorDuration(CreateHgramCounter(handleName + "_error_time", HIGH_RESOLUTION_COUNTER_VALUES))
    , ServeStartedDuration(CreateHgramCounter(handleName + "_serve_started_time", HIGH_RESOLUTION_COUNTER_VALUES))
    , ServiceUnavailable(CreateCounter(handleName + "_service_unavailable"), ErrorDuration)
    , Failed(FailedCounter, CreateHgramCounter(handleName + "_failed_time", HIGH_RESOLUTION_COUNTER_VALUES))
    , Timeout(TimeoutCounter, CreateHgramCounter(handleName + "_timeout_time", HIGH_RESOLUTION_COUNTER_VALUES))
    , NotFound(handleName + "_not_found")
    , Ok(handleName + "_ok")
    , ActiveSessionsCount(CreateAbsCounter(handleName + "_active_sessions_count"))
    , ActiveSessionsLimit(CreateAbsCounter(handleName + "_active_sessions_limit"))
{
}

void TRequestMetrics::OnStarted() {
    Increment(RequestsTotalCounter);
}

void TRequestMetrics::OnServeStarted(const TInstant startTime) {
    Measure(ServeStartedDuration, startTime);
}

void TRequestMetrics::OnOk(const TInstant startTime) {
    Ok.OnEvent(startTime);
}

void TRequestMetrics::OnFailed(const TInstant startTime) {
    Failed.OnEvent(startTime);
}

void TRequestMetrics::OnTimeout(const TInstant startTime) {
    Timeout.OnEvent(startTime);
}

void TRequestMetrics::OnUnexpected() {
    Increment(UnexpectedCounter);
}

void TRequestMetrics::OnNotFound(const TInstant startTime) {
    NotFound.OnEvent(startTime);
}

void TRequestMetrics::OnServiceUnavailable(const TInstant startTime) {
    ServiceUnavailable.OnEvent(startTime);
}

void TRequestMetrics::OnBadData(const TInstant startTime) {
    Increment(InvalidRequestCounter);
    Increment(FailedCounter);
    Measure(ErrorDuration, startTime);
}

void TRequestMetrics::OnInternalError(const TInstant startTime) {
    Increment(InternalErrorCounter);
    Increment(FailedCounter);
    Measure(ErrorDuration, startTime);
}

void TRequestMetrics::OnQueueFull(const TInstant startTime) {
    Increment(QueueFullErrorCounter);
    Increment(FailedCounter);
    Measure(ErrorDuration, startTime);
}

void TRequestMetrics::SetActiveSessionsCount(ui64 n) {
    ActiveSessionsCount->PushSignal(n);
}

void TRequestMetrics::SetActiveSessionsLimit(ui64 n) {
    ActiveSessionsLimit->PushSignal(n);
}


TCacheServiceMetrics::TCacheServiceMetrics()
    : Imdb("cache")
    , Request("cache")
    , YdbGet("ydb_get")
    , YdbSet("ydb_set")
    , YdbDel("ydb_del")
    , BackgroundTaskCompletedCounter(CreateCounter("cache_service_background_task_completed"))
    , BackgroundTaskAddFailCounter(CreateCounter("cache_service_background_task_add_fail"))
    , BackgroundTaskAddOkCounter(CreateCounter("cache_service_background_task_add_ok"))
    , LoadedDataSizeHgram(CreateHgramCounter("cache_service_loaded_size", SIZE_COUNTER_VALUES))
    , StoredDataSizeHgram(CreateHgramCounter("cache_service_stored_size", SIZE_COUNTER_VALUES))
    , CacheHit("cache_service_cache_hit")
    , CacheMiss("cache_service_cache_miss")
{
}

void TCacheServiceMetrics::OnBackgroundTaskCompleted() {
    Increment(BackgroundTaskCompletedCounter);
}

void TCacheServiceMetrics::OnBackgroundTaskAddFail() {
    Increment(BackgroundTaskAddFailCounter);
}

void TCacheServiceMetrics::OnBackgroundTaskAddOk() {
    Increment(BackgroundTaskAddOkCounter);
}

void TCacheServiceMetrics::LoadedSize(size_t size) {
    LoadedDataSizeHgram->PushSignal(size);
}

void TCacheServiceMetrics::StoredSize(size_t size) {
    StoredDataSizeHgram->PushSignal(size);
}

void TCacheServiceMetrics::OnCacheHit(const TInstant startTime) {
    CacheHit.OnEvent(startTime);
}

void TCacheServiceMetrics::OnCacheMiss(const TInstant startTime) {
    CacheMiss.OnEvent(startTime);
}


TActivationAlgorithmMetrics::TActivationAlgorithmMetrics()
    : TotalAnnouncements(CreateCounter("activation_total_announcements"))
    , TotalFinals(CreateCounter("activation_total_finals"))
    , TotalCleanups(CreateCounter("activation_total_cleanups"))
    , TotalLeaderFounds(CreateCounter("activation_total_leader_found"))
    , TotalLeaderNotFounds(CreateCounter("activation_total_leader_not_found"))
    , FailedAnnouncementCommits("activation_failed_announcement_commits")
    , SuccessfulAnnouncementCommits("activation_successful_announcement_commits")
    , WorthlessAnnouncementCommits("activation_worthless_announcement_commits")
    , BestSpotterWins("activation_best_spotter_wins")
    , BestSpotterFails("activation_best_spotter_fails")
    , OtherWins("activation_other_wins")
    , OtherFails("activation_other_fails")
{
}

void TActivationAlgorithmMetrics::OnAnnouncement() {
    Increment(TotalAnnouncements);
}

void TActivationAlgorithmMetrics::OnFinal() {
    Increment(TotalFinals);
}

void TActivationAlgorithmMetrics::OnCleanup() {
    Increment(TotalCleanups);
}

void TActivationAlgorithmMetrics::OnLeaderFound() {
    Increment(TotalLeaderFounds);
}

void TActivationAlgorithmMetrics::OnLeaderNotFound() {
    Increment(TotalLeaderNotFounds);
}

void TActivationAlgorithmMetrics::OnFailedAnnouncementCommit(const TInstant startTime) {
    FailedAnnouncementCommits.OnEvent(startTime);
}

void TActivationAlgorithmMetrics::OnSuccessfulAnnouncementCommit(const TInstant startTime) {
    SuccessfulAnnouncementCommits.OnEvent(startTime);
}

void TActivationAlgorithmMetrics::OnWorthlessAnnouncementCommit(const TInstant startTime) {
    WorthlessAnnouncementCommits.OnEvent(startTime);
}

void TActivationAlgorithmMetrics::OnWin(const TInstant startTime, bool isBestSpotter) {
    if (isBestSpotter) {
        BestSpotterWins.OnEvent(startTime);
    } else {
        OtherWins.OnEvent(startTime);
    }
}

void TActivationAlgorithmMetrics::OnFail(const TInstant startTime, bool isBestSpotter) {
    if (isBestSpotter) {
        BestSpotterFails.OnEvent(startTime);
    } else {
        OtherFails.OnEvent(startTime);
    }
}

TActivationMetrics::TActivationMetrics()
    : RequestMetrics("activation")
    , AlgorithmMetrics()
    , YdbMetrics("activation")
{
}

TGDPRMetrics::TGDPRMetrics()
    : RequestMetrics("gdpr")
    , YdbMetrics("gdpr")
{
}


TTakeoutMetrics::TTakeoutMetrics()
    : RequestMetrics("takeout")
    , YdbMetrics("takeout")
    , TextsInsertLengthCounter(CreateCounter("takeout_insert_texts_length"))
    , TextsInsertNumCounter(CreateCounter("takeout_insert_texts_num"))
    , TextsSelectLengthCounter(CreateCounter("takeout_select_texts_length"))
    , TextsSelectNumCounter(CreateCounter("takeout_select_texts_num"))
{
}

void TTakeoutMetrics::OnTextsInsert(size_t textsNum, size_t textsLength) {
    TextsInsertLengthCounter->PushSignal(textsLength);
    TextsInsertNumCounter->PushSignal(textsNum);
}

void TTakeoutMetrics::OnTextsSelect(size_t textsNum, size_t textsLength) {
    TextsSelectLengthCounter->PushSignal(textsLength);
    TextsSelectNumCounter->PushSignal(textsNum);
}


TMegamindSessionSubMetrics::TMegamindSessionSubMetrics(const TString& subType)
    : Load("mm_session_" + subType + "_load")
    , Save("mm_session_" + subType + "_save")
    , RequestMetrics("mm_session_" + subType + "_common")
{
}

TMegamindSessionSubMetrics::TSubSubMetrics::TSubSubMetrics(const TString& subSubType)
    : Total(subSubType + "_total")
    , Ok(subSubType + "_ok")
    , InternalError(subSubType + "_internal_error")
    , YdbOk(subSubType + "_ydb_ok")
    , YdbError(subSubType + "_ydb_error")
    , SizeHgram(CreateHgramCounter(subSubType + "_size", SIZE_COUNTER_VALUES))
    , CompressedSizeHgram(CreateHgramCounter(subSubType + "_compressed_size", SIZE_COUNTER_VALUES))
{
}

void TMegamindSessionSubMetrics::TSubSubMetrics::OnStart(const TInstant startTime) {
    Total.OnEvent(startTime);
}

void TMegamindSessionSubMetrics::TSubSubMetrics::OnOk(const TInstant startTime) {
    Ok.OnEvent(startTime);
}

void TMegamindSessionSubMetrics::TSubSubMetrics::OnInternalError(const TInstant startTime) {
    InternalError.OnEvent(startTime);
}

void TMegamindSessionSubMetrics::TSubSubMetrics::OnYdbOk(const TInstant startTime) {
    YdbOk.OnEvent(startTime);
}

void TMegamindSessionSubMetrics::TSubSubMetrics::OnYdbError(const TInstant startTime) {
    YdbError.OnEvent(startTime);
}

void TMegamindSessionSubMetrics::TSubSubMetrics::SetSize(size_t size) {
    SizeHgram->PushSignal(size);
}

void TMegamindSessionSubMetrics::TSubSubMetrics::SetCompressedSize(size_t size) {
    CompressedSizeHgram->PushSignal(size);
}


TMegamindSessionSubMetrics::TSubLoadMetrics::TSubLoadMetrics(const TString& subSubType)
    : TSubSubMetrics(subSubType)
    , NotFound(subSubType + "_not_found")
{
}

void TMegamindSessionSubMetrics::TSubLoadMetrics::OnNotFound(const TInstant startTime) {
    NotFound.OnEvent(startTime);
}

TMegamindSessionMetrics::TMegamindSessionMetrics()
    : Apphost("apphost")
    , Http("http")
    , Imdb("mm_session")
    , YdbGetMetrics("mm_session_ydb_get")
    , YdbSetMetrics("mm_session_ydb_set")
{
}


TYabioContextServiceMetrics::TYabioContextServiceMetrics()
    : Request("yabio_context")
    , YdbDel("yabio_context_ydb_del")
    , YdbGet("yabio_context_ydb_get")
    , YdbSet("yabio_context_ydb_set")
{
}


TMetrics::TMetrics()
    : ActivationMetrics()
    , VinsContextRequestMetrics("vins_context")
    , RedisGetMetrics("redis_get")
    , RedisSetMetrics("redis_set")
{
}


TMetrics& TMetrics::GetInstance() {
    return *Singleton<TMetrics>();
}

}   // namespace NCachalot

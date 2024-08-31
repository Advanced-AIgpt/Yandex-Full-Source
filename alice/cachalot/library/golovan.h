#pragma once

#include <library/cpp/unistat/unistat.h>

#include <util/datetime/base.h>
#include <util/generic/string.h>
#include <util/generic/map.h>


namespace NCachalot {

class IEventMetric {
public:
    virtual ~IEventMetric() = default;

    virtual void OnEvent(const TInstant startTime) = 0;
};

class TEventMetricBase : public IEventMetric {
public:
    explicit TEventMetricBase(const TString& eventName);
    explicit TEventMetricBase(NUnistat::IHolePtr eventCounter);

    virtual void OnEvent(const TInstant startTime);

protected:
    NUnistat::IHolePtr EventCounter;
};

class TEventMetric : public TEventMetricBase {
public:
    explicit TEventMetric(const TString& eventName);
    TEventMetric(NUnistat::IHolePtr eventCounter, NUnistat::IHolePtr durationConter);

    void OnEvent(const TInstant startTime) override;

protected:
    NUnistat::IHolePtr DurationConter;
};


/*
    Class TYdbMetrics contains counters for all possible ydb responses and scenarios.
    Some of the counters may be unused in some scenarios. It's ok.
*/
class TYdbMetrics {
public:
    explicit TYdbMetrics(const TString& operationName);

    void OnExecuteError(const TInstant startTime);
    void OnNotFound(const TInstant startTime);
    void OnOk(const TInstant startTime);
    void OnOperationStarted(const TInstant startTime);
    void OnRetryError(const TInstant startTime);
    void OnAssertionError(const TInstant startTime);
    void OnExecuteResponse(const TInstant startTime);
    void OnExecuteException(const TInstant startTime);

private:
    TEventMetric ExecuteError;
    TEventMetric NotFound;
    TEventMetric Ok;
    TEventMetric OperationStarted;
    TEventMetric RetryError;
    TEventMetric AssertionError;
    TEventMetric ExecuteResponse;
    TEventMetric ExecuteException;
};


class TRedisMetrics {
public:
    explicit TRedisMetrics(const TString& operationName);

    void OnError(const TInstant startTime);
    void OnOk(const TInstant startTime);

protected:
    TEventMetric Error;
    TEventMetric Ok;
};


class TRedisGetMetrics : public TRedisMetrics {
public:
    explicit TRedisGetMetrics(const TString& operationName);

    void OnCacheHit(const TInstant startTime);
    void OnCacheMiss(const TInstant startTime);

private:
    TEventMetric CacheHit;
    TEventMetric CacheMiss;
};


class TInmemoryStorageOperationMetricsBase {
public:
    explicit TInmemoryStorageOperationMetricsBase(const TString& prefix);

    void OnOk(const TInstant startTime);
    void OnFailed(const TInstant startTime);
    void OnQueueFull();

private:
    TEventMetric Ok;
    TEventMetric Failed;
    NUnistat::IHolePtr QueueFullErrorCounter;
};


class TInmemoryStorageLoadMetrics : public TInmemoryStorageOperationMetricsBase {
public:
    explicit TInmemoryStorageLoadMetrics(const TString& prefix);

    void OnNotFound(const TInstant startTime);

private:
    TEventMetric NotFound;
};


class TInmemoryStorageStoreMetrics : public TInmemoryStorageOperationMetricsBase {
public:
    explicit TInmemoryStorageStoreMetrics(const TString& prefix);
};


class TInmemoryStorageRemoveMetrics : public TInmemoryStorageOperationMetricsBase {
public:
    explicit TInmemoryStorageRemoveMetrics(const TString& prefix);

    void OnNotFound(const TInstant startTime);

private:
    TEventMetric NotFound;
};


class TInmemoryStorageMetrics {
public:
    explicit TInmemoryStorageMetrics(const TString& handleName);

    void SetMemoryUsage(uint64_t memoryUsageBytes);
    void SetNumberOfItems(uint64_t numberOfItems);

public:
    TInmemoryStorageLoadMetrics Get;
    TInmemoryStorageStoreMetrics Set;
    TInmemoryStorageRemoveMetrics Delete;

private:
    NUnistat::IHolePtr MemoryUsage;
    NUnistat::IHolePtr NumberOfItems;
};


class TInmemoryStorageMetricsContainer {
public:
    explicit TInmemoryStorageMetricsContainer(TString handleName);

    TInmemoryStorageMetrics& operator[](const TString& storageTag);

private:
    TString HandleName;
    TMap<TString, TInmemoryStorageMetrics> StorageTag2Metrics;
};


class TRequestMetrics {
public:
    explicit TRequestMetrics(const TString& handleName);

    void OnStarted();
    void OnServeStarted(const TInstant startTime);
    void OnOk(const TInstant startTime);
    void OnFailed(const TInstant startTime);
    void OnTimeout(const TInstant startTime);
    void OnNotFound(const TInstant startTime);
    void OnServiceUnavailable(const TInstant startTime);
    void OnBadData(const TInstant startTime);
    void OnInternalError(const TInstant startTime);
    void OnQueueFull(const TInstant startTime);
    void OnUnexpected();

    void SetActiveSessionsCount(ui64);
    void SetActiveSessionsLimit(ui64);

private:
    NUnistat::IHolePtr InternalErrorCounter;
    NUnistat::IHolePtr InvalidRequestCounter;
    NUnistat::IHolePtr QueueFullErrorCounter;
    NUnistat::IHolePtr RequestsTotalCounter;
    NUnistat::IHolePtr FailedCounter;
    NUnistat::IHolePtr TimeoutCounter;
    NUnistat::IHolePtr UnexpectedCounter;

    NUnistat::IHolePtr ErrorDuration;
    NUnistat::IHolePtr ServeStartedDuration;

    TEventMetric ServiceUnavailable;
    TEventMetric Failed;
    TEventMetric Timeout;
    TEventMetric NotFound;
    TEventMetric Ok;

    NUnistat::IHolePtr ActiveSessionsCount;
    NUnistat::IHolePtr ActiveSessionsLimit;
};


class TActivationAlgorithmMetrics {
public:
    explicit TActivationAlgorithmMetrics();

    void OnAnnouncement();
    void OnFinal();

    void OnCleanup();
    void OnLeaderFound();
    void OnLeaderNotFound();

    void OnFailedAnnouncementCommit(const TInstant startTime);
    void OnSuccessfulAnnouncementCommit(const TInstant startTime);
    void OnWorthlessAnnouncementCommit(const TInstant startTime);

    void OnWin(const TInstant startTime, bool isBestSpotter);
    void OnFail(const TInstant startTime, bool isBestSpotter);

private:
    NUnistat::IHolePtr TotalAnnouncements;
    NUnistat::IHolePtr TotalFinals;
    NUnistat::IHolePtr TotalCleanups;
    NUnistat::IHolePtr TotalLeaderFounds;
    NUnistat::IHolePtr TotalLeaderNotFounds;

    TEventMetric FailedAnnouncementCommits;  // when ydb fails
    TEventMetric SuccessfulAnnouncementCommits;  // when all is ok
    TEventMetric WorthlessAnnouncementCommits;  // when spotter loses on first stage

    TEventMetric BestSpotterWins;
    TEventMetric BestSpotterFails;
    TEventMetric OtherWins;
    TEventMetric OtherFails;
};


struct TActivationMetrics {
public:
    TActivationMetrics();

public:
    TRequestMetrics RequestMetrics;
    TActivationAlgorithmMetrics AlgorithmMetrics;
    TYdbMetrics YdbMetrics;
};


class TCacheServiceMetrics {
public:
    explicit TCacheServiceMetrics();

    void OnBackgroundTaskCompleted();
    void OnBackgroundTaskAddFail();
    void OnBackgroundTaskAddOk();

    void LoadedSize(size_t);
    void StoredSize(size_t);

    void OnCacheHit(const TInstant startTime);
    void OnCacheMiss(const TInstant startTime);

public:
    TInmemoryStorageMetricsContainer Imdb;
    TRequestMetrics Request;
    TYdbMetrics YdbGet;
    TYdbMetrics YdbSet;
    TYdbMetrics YdbDel;

private:
    NUnistat::IHolePtr BackgroundTaskCompletedCounter;
    NUnistat::IHolePtr BackgroundTaskAddFailCounter;
    NUnistat::IHolePtr BackgroundTaskAddOkCounter;

    NUnistat::IHolePtr LoadedDataSizeHgram;
    NUnistat::IHolePtr StoredDataSizeHgram;

    TEventMetric CacheHit;
    TEventMetric CacheMiss;
};


class TGDPRMetrics {
public:
    TGDPRMetrics();

public:
    TRequestMetrics RequestMetrics;
    TYdbMetrics YdbMetrics;
};


class TTakeoutMetrics {
public:
    TTakeoutMetrics();

    void OnTextsInsert(size_t, size_t);
    void OnTextsSelect(size_t, size_t);
public:
    TRequestMetrics RequestMetrics;
    TYdbMetrics YdbMetrics;
    NUnistat::IHolePtr TextsInsertLengthCounter;
    NUnistat::IHolePtr TextsInsertNumCounter;
    NUnistat::IHolePtr TextsSelectLengthCounter;
    NUnistat::IHolePtr TextsSelectNumCounter;
};


struct TMegamindSessionSubMetrics {
public:
    struct TSubSubMetrics {
    public:
        TEventMetric Total;             // total count
        TEventMetric Ok;                // ok count + ok hgram
        TEventMetric InternalError;     // cachalot error + cachalot error hgram
        TEventMetric YdbOk;             // ydb ok + ydb ok hgram
        TEventMetric YdbError;          // ydb error + ydb error hgram
        NUnistat::IHolePtr SizeHgram;   // size hgram
        NUnistat::IHolePtr CompressedSizeHgram;   // compressed size hgram

    public:
        explicit TSubSubMetrics(const TString& subSubType);

        void OnStart(const TInstant startTime);
        void OnOk(const TInstant startTime);
        void OnInternalError(const TInstant startTime);
        void OnYdbOk(const TInstant startTime);
        void OnYdbError(const TInstant startTime);
        void SetSize(size_t size);
        void SetCompressedSize(size_t size);

    };

    struct TSubLoadMetrics : public TSubSubMetrics {
    public:
        TEventMetric NotFound;

    public:
        explicit TSubLoadMetrics(const TString& subSubType);

        void OnNotFound(const TInstant startTime);
    };

public:
    explicit TMegamindSessionSubMetrics(const TString& subType);

public:
    TSubLoadMetrics Load;
    TSubSubMetrics Save;
    TRequestMetrics RequestMetrics;
};


struct TMegamindSessionMetrics {
public:
    TMegamindSessionMetrics();

public:
    TMegamindSessionSubMetrics Apphost;
    TMegamindSessionSubMetrics Http;

    TInmemoryStorageMetrics Imdb;

    TYdbMetrics YdbGetMetrics;
    TYdbMetrics YdbSetMetrics;
};


struct TYabioContextServiceMetrics {
public:
    TYabioContextServiceMetrics();

public:
    TRequestMetrics Request;
    TYdbMetrics YdbDel;
    TYdbMetrics YdbGet;
    TYdbMetrics YdbSet;
};


struct TMetrics {
public:
    TRequestMetrics* VinsContextMetrics() {
        return &VinsContextRequestMetrics;
    }

    static TMetrics& GetInstance();

    TMetrics();

public:
    TActivationMetrics ActivationMetrics;
    TGDPRMetrics GDPRMetrics;
    TTakeoutMetrics TakeoutMetrics;
    TRequestMetrics VinsContextRequestMetrics;

    TRedisGetMetrics RedisGetMetrics;
    TRedisMetrics RedisSetMetrics;

    TCacheServiceMetrics CacheServiceMetrics;
    TMegamindSessionMetrics MegamindSessionMetrics;
    TYabioContextServiceMetrics YabioContextServiceMetrics;
};

}   // namespace NCachalot

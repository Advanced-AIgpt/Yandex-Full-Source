#include <alice/cachalot/library/storage/redis.h>

#include <alice/cachalot/library/golovan.h>
#include <alice/cachalot/library/storage/mock.h>
#include <alice/cachalot/library/utils.h>

#include <alice/cuttlefish/library/redis/redis.h>


namespace NCachalot {


NRedis::TContextPtr TRedisPool::GetOrCreateContext() {
    {
        TReadGuard lock(Mut);
        auto it = Conns.find(TThread::CurrentThreadId());
        if (it != Conns.end()) {
            NRedis::TContextPtr ptr = it->second;
            if (ptr.Get() && *ptr.Get()) {
                return ptr;
            }
        }
    }

    {
        TWriteGuard lock(Mut);
        NRedis::TContextPtr ptr = NRedis::MakeContext("127.0.0.1", Settings.Port());
        Conns.insert(
            std::make_pair(TThread::CurrentThreadId(), ptr)
        );
        return ptr;
    }

    return nullptr;
}


template <typename TResultType>
class TRedisOperationBase : public TThrRefBase {
public:
    using TSelf = TRedisOperationBase;
    using TResult = TResultType;

public:
    explicit TRedisOperationBase(
        TRedisPool* redisPool
    )
        : Response(NThreading::NewPromise<TResult>())
        , RedisPool(redisPool)
    {
    }

    NThreading::TPromise<TResult> GetResponse() {
        return Response;
    }

    void OnStart() {
        StartTime = TInstant::Now();
    }

    virtual void OnError() = 0;

    void Execute() {
        TResult result;
        result.Stats.SchedulingTime = MillisecondsSince(StartTime);
        result
            .SetKey(Key)
            .SetStatus(EResponseStatus::QUERY_PREPARE_FAILED);

        NRedis::TContextPtr context = RedisPool->GetOrCreateContext();

        if (!context.Get()->IsValid()) {
            result.SetStatus(EResponseStatus::BAD_GATEWAY);
            Response.SetValue(result);
            return;
        }

        const NRedis::TReply value = MakeRedisRequest(context);
        result.Stats.FetchingTime = MillisecondsSince(StartTime);
        Response.SetValue(ProcessRedisReply(value));
    }

    TSelf& SetKey(TString key) {
        Key = std::move(key);
        return *this;
    }

    TSelf& SetData(TString data) {
        Data = std::move(data);
        return *this;
    }

private:
    virtual TResult ProcessRedisReply(const NRedis::TReply& reply) = 0;
    virtual NRedis::TReply MakeRedisRequest(NRedis::TContextPtr context) = 0;

protected:
    NThreading::TPromise<TResult> Response;
    TRedisPool* RedisPool;

    TInstant StartTime;
    TStorageStats Stats;

    TString Key;
    TString Data;
};


class TRedisGetOperation : public TRedisOperationBase<TRedisStorage::TSingleRowResponse> {
public:
    using TBase = TRedisOperationBase<TRedisStorage::TSingleRowResponse>;

    explicit TRedisGetOperation(
        TRedisPool* redisPool
    )
        : TBase(redisPool)
        , Metrics(&TMetrics::GetInstance().RedisGetMetrics)
    {
    }

public:
    void OnError() override {
        Metrics->OnError(StartTime);
    }

protected:
    TRedisStorage::TSingleRowResponse ProcessRedisReply(const NRedis::TReply& reply) override {
        TRedisStorage::TSingleRowResponse result;
        result.SetKey(Key);

        if (reply.IsNil()) {
            Metrics->OnCacheMiss(StartTime);
            result
                .SetStatus(EResponseStatus::NO_CONTENT);
        } else if (reply.IsString()) {
            Metrics->OnCacheHit(StartTime);
            result
                .SetData(reply.GetData())
                .SetStatus(EResponseStatus::OK);
        } else if (reply.IsError()) {
            Metrics->OnError(StartTime);
            result
                .SetError(reply.GetData())
                .SetStatus(EResponseStatus::QUERY_EXECUTE_FAILED);
        } else {
            Metrics->OnError(StartTime);
            result
                .SetError("Weird response from Redis")
                .SetStatus(EResponseStatus::INTERNAL_ERROR);
        }

        return result;
    }

    NRedis::TReply MakeRedisRequest(NRedis::TContextPtr context) override {
        return context->Execute("GET %b", Key.data(), Key.size());
    }

private:
    TRedisGetMetrics* Metrics;
};

class TRedisSetOperation : public TRedisOperationBase<TRedisStorage::TEmptyResponse> {
public:
    using TBase = TRedisOperationBase<TRedisStorage::TEmptyResponse>;

    explicit TRedisSetOperation(
        TRedisPool* redisPool
    )
        : TBase(redisPool)
        , Metrics(&TMetrics::GetInstance().RedisSetMetrics)
    {
    }

public:
    TRedisSetOperation& SetTtl(int64_t ttlSeconds) {
        TtlSeconds = ttlSeconds;
        return *this;
    }

    void OnError() override {
        Metrics->OnError(StartTime);
    }

protected:
    TRedisStorage::TEmptyResponse ProcessRedisReply(const NRedis::TReply& reply) override {
        TRedisStorage::TEmptyResponse result;
        result.SetKey(Key);

        if (reply.IsNil()) {
            Metrics->OnError(StartTime);
            result
                .SetError("Redis SET -> NIL")
                .SetStatus(EResponseStatus::INTERNAL_ERROR);
        } else if (reply.IsStatus()) {
            Metrics->OnOk(StartTime);
            result
                .SetStatus(EResponseStatus::CREATED);
        } else if (reply.IsError()) {
            Metrics->OnError(StartTime);
            result
                .SetError(reply.GetData())
                .SetStatus(EResponseStatus::QUERY_EXECUTE_FAILED);
        } else {
            Metrics->OnError(StartTime);
            result
                .SetError("Weird response from Redis")
                .SetStatus(EResponseStatus::INTERNAL_ERROR);
        }

        return result;
    }

    NRedis::TReply MakeRedisRequest(NRedis::TContextPtr context) override {
        if (TtlSeconds <= 0) {
            return context->Execute("SET %b %b", Key.data(), Key.size(), Data.data(), Data.size());
        } else {
            return context->Execute("SET %b %b EX %d", Key.data(), Key.size(), Data.data(), Data.size(), TtlSeconds);
        }
    }

private:
    TRedisMetrics* Metrics;
    int64_t TtlSeconds = -1;
};


template <typename TRedisOperationType>
class TRedisOperationExecutor {
public:
    using TResult = typename TRedisOperationType::TResult;

public:
    explicit TRedisOperationExecutor(
        TIntrusivePtr<TRedisOperationType> operation
    )
        : Operation(std::move(operation))
    {
    }

    NThreading::TFuture<TResult> ExecuteInPool(TThreadPool* threadPool) {
        Operation->OnStart();

        if (!threadPool->AddFunc(*this)) {
            Operation->OnError();
            return NThreading::MakeFuture(
                TResult()
                    .SetStatus(EResponseStatus::TOO_MANY_REQUESTS)
            );
        }

        return Operation->GetResponse();
    }

    void operator()() {
        Operation->Execute();
    }

private:
    TIntrusivePtr<TRedisOperationType> Operation;
};


TRedisStorage::TRedisStorage(const NCachalot::TRedisSettings& settings)
    : Settings(settings)
    , ThreadPool(TThreadPoolParams("redis-servant"))
    , Conns(Settings)
{
    ThreadPool.Start(Settings.PoolSize(), Settings.QueueSize());
}


TRedisStorage::~TRedisStorage()
{ }


NThreading::TFuture<TRedisStorage::TEmptyResponse> TRedisStorage::Set(
    const TString& key,
    const TString& data,
    int64_t ttl,
    TChroniclerPtr /* chronicler */
) {
    TIntrusivePtr<TRedisSetOperation> operation = MakeIntrusive<TRedisSetOperation>(&Conns);
    operation->SetTtl(ttl);
    operation->SetKey(key);
    operation->SetData(data);
    return TRedisOperationExecutor(std::move(operation)).ExecuteInPool(&ThreadPool);
}


NThreading::TFuture<TRedisStorage::TSingleRowResponse> TRedisStorage::GetSingleRow(
    const TString& key, TChroniclerPtr /* chronicler */
) {
    TIntrusivePtr<TRedisGetOperation> operation = MakeIntrusive<TRedisGetOperation>(&Conns);
    operation->SetKey(key);
    return TRedisOperationExecutor(std::move(operation)).ExecuteInPool(&ThreadPool);
}


NThreading::TFuture<TRedisStorage::TMultipleRowsResponse> TRedisStorage::Get(
    const TString& key, TChroniclerPtr /* chronicler */
) {
    return NThreading::MakeFuture(
        TMultipleRowsResponse()
            .SetStatus(EResponseStatus::NOT_IMPLEMENTED)
            .SetKey(key)
            .SetError("IStorage::Get(MultipleRows) method is not implemeted for redis")
    );
}


NThreading::TFuture<TRedisStorage::TEmptyResponse> TRedisStorage::Del(
    const TString& key, TChroniclerPtr /* chronicler */
) {
    return NThreading::MakeFuture(
        TEmptyResponse()
            .SetStatus(EResponseStatus::NOT_IMPLEMENTED)
            .SetKey(key)
            .SetError("IStorage::Del method is not implemeted for redis")
    );
}


TIntrusivePtr<IKvStorage<TString, TString>> MakeSimpleRedisStorage(const NCachalot::TRedisSettings& settings) {
    return MakeStorage<TRedisStorage>(settings.IsFake(), settings);
}

}

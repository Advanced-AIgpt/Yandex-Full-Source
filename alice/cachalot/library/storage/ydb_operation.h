#pragma once

#include <alice/cachalot/library/debug.h>
#include <alice/cachalot/library/golovan.h>
#include <alice/cachalot/library/storage/storage.h>
#include <alice/cachalot/library/storage/ydb.h>
#include <alice/cachalot/library/utils.h>

#include <alice/cachalot/events/cachalot.ev.pb.h>
#include <alice/cuttlefish/library/logging/dlog.h>

#include <ydb/public/sdk/cpp/client/ydb_types/fluent_settings_helpers.h>


namespace NCachalot {

struct TYdbOperationSettings {
    using TSelf = TYdbOperationSettings;

    FLUENT_SETTING_DEFAULT(TDuration, ClientTimeout, TDuration::MilliSeconds(350));
    FLUENT_SETTING_DEFAULT(TDuration, OperationTimeout, TDuration::MilliSeconds(250));
    FLUENT_SETTING_OPTIONAL(TDuration, CancelAfter);
    FLUENT_SETTING_DEFAULT(int, MaxRetries, 1);
};


/*
    This base class implements basic logic of Execute function: metrics, retries and query formating.
    Derived class should implement following methods:
        * GetQueryTemplate
        * BuildQueryParams
        * ProcessQueryResult
*/
template<typename TResponse, typename TKey, typename TData=void>
class TYdbOperationBase : public TThrRefBase {
public:
    using TSelf = TYdbOperationBase<TResponse, TKey, TData>;

public:
    explicit TYdbOperationBase(TYdbMetrics* ydbMetrics)
        : Metrics(ydbMetrics)
    {
    }

    virtual ~TYdbOperationBase() = default;

    TSelf& SetKey(TKey key) {
        Key = std::move(key);
        return *this;
    }

    TSelf& SetDatabase(TString databaseName) {
        DatabaseName = std::move(databaseName);
        return *this;
    }

    TSelf& SetTable(TString tableName) {
        TableName = std::move(tableName);
        return *this;
    }

    TSelf& SetLogger(TChroniclerPtr logFrame) {
        LogFrame = logFrame;
        return *this;
    }

    TSelf& SetOperationSettings(const TYdbOperationSettingsBase& settings) {
        OperationSettings.OperationTimeout_ = TDuration::MilliSeconds(settings.TimeoutMilliseconds());
        OperationSettings.ClientTimeout_ = OperationSettings.OperationTimeout_ + TDuration::MilliSeconds(100);
        OperationSettings.MaxRetries_ = settings.MaxRetriesCount();
        return *this;
    }

    TYdbOperationSettings& MutableOperationSettings() {
        return OperationSettings;
    }

    NThreading::TFuture<TResponse> Execute(NYdb::NTable::TTableClient* ydbClent);

protected:
    virtual TString GetQueryTemplate() const = 0;

    virtual NYdb::TParams BuildQueryParams(NYdb::TParamsBuilder paramsBuilder) const = 0;

    virtual void ProcessQueryResult(
        NThreading::TPromise<NYdb::TStatus>* status,
        const NYdb::NTable::TDataQueryResult& queryResult) = 0;

    virtual NYdb::NTable::TTxSettings GetTransactionMode() const {
        return NYdb::NTable::TTxSettings::SerializableRW();
    }

    virtual NYdb::NTable::TExecDataQuerySettings GetExecDataQuerySettings() const {
        return GetYdbRequestSettings<NYdb::NTable::TExecDataQuerySettings>().KeepInQueryCache(true);
    }

    virtual bool IsAssertionError(const NYql::TIssues& /* issues */) const {
        return false;
    }

    TIntrusivePtr<TSelf> IntrusiveThis() {
        return this;
    }

    template <typename TEvent, typename... TArgs>
    void Log(TArgs&&... args) {
        if (LogFrame) {
            LogFrame->Log().LogEventInfoCombo<TEvent>(std::forward<TArgs>(args)...);
        }
    }

private:
    template<typename TSettings>
    TSettings GetYdbRequestSettings() const {
        return TSettings()
            .ClientTimeout(OperationSettings.ClientTimeout_)
            .OperationTimeout(OperationSettings.OperationTimeout_)
            .CancelAfter(OperationSettings.CancelAfter_.GetOrElse(OperationSettings.OperationTimeout_));
    }

protected:
    TYdbMetrics* Metrics;
    TChroniclerPtr LogFrame;
    TInstant StartTime;
    TStorageStats Stats;

    TKey Key;
    TString DatabaseName;
    TString TableName;  // Optional. For some modules table name is hardcoded in yql queries.
    TYdbOperationSettings OperationSettings;

    // this field stores the response we received during last retry attempt.
    TResponse LastResponse;
};


template<typename TKey, typename TData>
class TYdbSetOperationBase : public TYdbOperationBase<TEmptyStorageResponse<TKey>, TKey, TData> {
public:
    using TResponse = TEmptyStorageResponse<TKey>;
    using TBase = TYdbOperationBase<TResponse, TKey, TData>;
    using TSelf = TYdbSetOperationBase<TKey, TData>;

    using TBase::TBase;

    TSelf& SetTtl(int64_t seconds) {
        TtlSeconds = seconds;
        return *this;
    }

    TSelf& SetData(TData data) {
        Data = std::move(data);
        return *this;
    }

protected:
    void ProcessQueryResult(
        NThreading::TPromise<NYdb::TStatus>* status,
        const NYdb::NTable::TDataQueryResult& queryResult
    ) override;

protected:
    TData Data;
    int64_t TtlSeconds = -1;
};


template<typename TKey, typename TData=void>
class TYdbDeleteOperationBase : public TYdbOperationBase<TEmptyStorageResponse<TKey>, TKey, TData> {
public:
    using TResponse = TEmptyStorageResponse<TKey>;
    using TBase = TYdbOperationBase<TResponse, TKey, TData>;
    using TSelf = TYdbDeleteOperationBase<TKey, TData>;

    using TBase::TBase;

protected:
    void ProcessQueryResult(
        NThreading::TPromise<NYdb::TStatus>* status,
        const NYdb::NTable::TDataQueryResult& queryResult
    ) override;
};


template<typename TResponse, typename TKey, typename TData>
NThreading::TFuture<TResponse> TYdbOperationBase<TResponse, TKey, TData>::Execute(
    NYdb::NTable::TTableClient* ydbClent
) {
    NThreading::TPromise<TResponse> response = NThreading::NewPromise<TResponse>();

    if (!ydbClent) {
        response.SetValue(
            TResponse()
                .SetStatus(EResponseStatus::BAD_GATEWAY)
                .SetKey(Key)
                .SetError("YDB == NULL")
        );
        return response;
    }

    Log<NEvClass::YdbOperationStart>();
    StartTime = TInstant::Now();

    LastResponse.SetKey(Key);

    // Here we pass two pointers to the lambda.
    // `operation=IntrusiveThis()` and `this` are pointing to the same object.
    // We need `operation` for memory managment: object will live as long, as we need during async processing.
    // `this` allows us to use private members and helps to avoid ugly boilerplate like `operation->`.
    ydbClent->RetryOperation(
        [operation=IntrusiveThis(), this](NYdb::NTable::TSession session) mutable {
            Stats.SchedulingTime = MillisecondsSince(StartTime);

            Metrics->OnOperationStarted(StartTime);
            Log<NEvClass::YdbOperationOperationStarted>();

            NThreading::TPromise<NYdb::TStatus> status = NThreading::NewPromise<NYdb::TStatus>();

            session.ExecuteDataQuery(
                GetQueryTemplate(),
                NYdb::NTable::TTxControl::BeginTx(GetTransactionMode()).CommitTx(),
                BuildQueryParams(session.GetParamsBuilder()),
                GetExecDataQuerySettings()
            ).Subscribe(
                [operation, this, status](NYdb::NTable::TAsyncDataQueryResult fut) mutable {
                    Stats.FetchingTime = MillisecondsSince(StartTime);

                    if (fut.HasException()) {
                        Metrics->OnExecuteException(StartTime);
                        try {
                            fut.TryRethrow();
                        } catch (...) {
                            Log<NEvClass::YdbOperationExecuteException>(CurrentExceptionMessage());
                        }
                    } else {
                        Metrics->OnExecuteResponse(StartTime);
                        Log<NEvClass::YdbOperationExecuteResponse>();
                        try {
                            const NYdb::NTable::TDataQueryResult& queryResult = fut.GetValueSync();
                            if (!queryResult.IsSuccess()) {
                                Stats.ErrorMessage = queryResult.GetIssues().ToString();
                                Log<NEvClass::YdbOperationExecuteError>(Stats.ErrorMessage);
                            }
                            ProcessQueryResult(&status, queryResult);
                        } catch (...) {
                            Log<NEvClass::YdbOperationHorribleMistake>(CurrentExceptionMessage());
                        }
                    }

                    LastResponse.SetStats(Stats);
                }
            );

            return status.GetFuture();
        },
        NYdb::NTable::TRetryOperationSettings().MaxRetries(OperationSettings.MaxRetries_)
    ).Subscribe(
        [this, operation=IntrusiveThis(), response](const NYdb::TAsyncStatus status) mutable {
            auto setError = [&] (TString error, EResponseStatus errorStatus) {
                response.TrySetValue(
                    TResponse()
                        .SetKey(Key)
                        .SetStatus(errorStatus)
                        .SetStats(MillisecondsSince(StartTime), 0)
                        .SetError(std::move(error))
                );
            };

            auto setRetryError = [&] (TString error) {
                setError(std::move(error), EResponseStatus::QUERY_EXECUTE_FAILED);
                Metrics->OnRetryError(StartTime);
                Log<NEvClass::YdbOperationRetryError>();
            };

            auto processIssues = [&] (const NYql::TIssues& issues) {
                TString error = issues.ToString();
                if (IsAssertionError(issues)) {
                    Log<NEvClass::YdbOperationAssertionError>(error);
                    setError(std::move(error), EResponseStatus::QUERY_ASSERTION_FAILED);
                    Metrics->OnAssertionError(StartTime);
                } else {
                    setRetryError(std::move(error));
                }
            };

            try {
                auto it = status.GetValueSync();
                if (!it.IsSuccess()) {
                    processIssues(it.GetIssues());
                } else {
                    response.SetValue(LastResponse);
                }
            } catch (...) {
                setRetryError("Exception while handling ydb response: " + CurrentExceptionMessage());
            }
        }
    );
    return response;
}


template<typename TKey, typename TData>
void TYdbSetOperationBase<TKey, TData>::ProcessQueryResult(
    NThreading::TPromise<NYdb::TStatus>* status,
    const NYdb::NTable::TDataQueryResult& queryResult
) {
    if (queryResult.IsSuccess()) {
        TBase::Metrics->OnOk(TBase::StartTime);
        TBase::template Log<NEvClass::YdbOperationOk>();

        TBase::LastResponse.SetStatus(EResponseStatus::CREATED);
    } else {
        TBase::Metrics->OnExecuteError(TBase::StartTime);
        // Issues are logged in line 260

        TBase::LastResponse.SetStatus(EResponseStatus::QUERY_EXECUTE_FAILED);
        TBase::LastResponse.SetError(queryResult.GetIssues().ToString());
    }
    status->SetValue(queryResult);
}


template<typename TKey, typename TData>
void TYdbDeleteOperationBase<TKey, TData>::ProcessQueryResult(
    NThreading::TPromise<NYdb::TStatus>* status,
    const NYdb::NTable::TDataQueryResult& queryResult
) {
    if (queryResult.IsSuccess()) {
        TBase::Metrics->OnOk(TBase::StartTime);
        TBase::template Log<NEvClass::YdbOperationOk>();
        TBase::LastResponse.SetStatus(EResponseStatus::OK);
    } else {
        TBase::Metrics->OnExecuteError(TBase::StartTime);
        TBase::LastResponse.SetStatus(EResponseStatus::QUERY_EXECUTE_FAILED);
        TBase::LastResponse.SetError(queryResult.GetIssues().ToString());
    }
    status->SetValue(queryResult);
}

}   // namespace NCachalot

#include "storage.h"

#include <util/string/cast.h>

namespace NMatrix {

IYDBStorage::IYDBStorage(
    const NYdb::TDriver& driver,
    const TYDBClientSettings& config,
    const TStringBuf name
)
    : Name_(name)
    , Client_(
        driver,
        NYdb::NTable::TClientSettings()
            // https://a.yandex-team.ru/arc/trunk/arcadia/kikimr/public/sdk/cpp/client/ydb_table.h?rev=r7836427#L717
            .UseQueryCache(false)
            .SessionPoolSettings(
                NYdb::NTable::TSessionPoolSettings()
                    .MaxActiveSessions(config.GetMaxActiveSessions())
            )
    )
    , RetryOperationSettings_(NYdb::NTable::TRetryOperationSettings().MaxRetries(config.GetMaxRetries()))
    , ExecDataQuerySettings_(
        NYdb::NTable::TExecDataQuerySettings()
            .OperationTimeout(FromString<TDuration>(config.GetOperationTimeout()))
            .ClientTimeout(FromString<TDuration>(config.GetClientTimeout()))
            .CancelAfter(FromString<TDuration>(config.GetCancelAfter()))
            .KeepInQueryCache(true)
    )
    , OperationSettings_({
        .ReportTimings = config.GetReportOperationsTimings()
    })
{}

TString IYDBStorage::YDBStatusToString(const NYdb::TStatus& res) {
    TString result;

    TStringOutput output(result);
    output << res.GetStatus() << Endl;
    res.GetIssues().PrintTo(output);

    return result;
}

TExpected<void, TString> IYDBStorage::DefaultResultChecker(const NYdb::TStatus& res) {
    if (res.IsSuccess()) {
        return TExpected<void, TString>::DefaultSuccess();
    } else {
        return IYDBStorage::YDBStatusToString(res);
    }
}

IYDBStorage::TOperationContext::TOperationContext(
    const TString& storageName,
    const TString& operationName,
    const TOperationSettings& operationSettings,
    TLogContext logContext,
    TSourceMetrics& metrics
)
    : StorageName(storageName)
    , OperationName(operationName)
    , OperationSettings(operationSettings)
    , FullOperationName(TString::Join(StorageName, '_', OperationName))
    , LogContext(logContext)
    , Metrics(metrics)
    , StartTime(TInstant::Now())
    , RtLogActivation(
        logContext.RtLogPtr()
        ? MakeAtomicShared<TRtLogActivation>(
            logContext.RtLogPtr(),
            TString::Join("ydb-", FullOperationName),
            /* newRequest = */ false
        )
        : MakeAtomicShared<TRtLogActivation>()
    )
{
    LogContext.LogEventInfoCombo<NEvClass::TMatrixYdbOperationStart>(
        StorageName,
        OperationName
    );
}

TExpected<void, TString> IYDBStorage::TOperationContext::ReportResult(
    const NYdb::TStatus& res,
    TResultChecker resultChecker
) {
    if (OperationSettings.ReportTimings) {
        Metrics.PushTimeDiffWithNowHist(
            StartTime,
            TString::Join(FullOperationName, "_request_time"),
            "", /* code */
            "ydb"
        );
    }

    auto checkResult = resultChecker(res);
    if (checkResult.IsSuccess()) {
        Metrics.PushRate(FullOperationName, "ok", "ydb");
        LogContext.LogEventInfoCombo<NEvClass::TMatrixYdbOperationSuccess>(StorageName, OperationName);
        // Do not finish RtLogActivation here
        // An error may occur after ReportResult in response parsing
    } else {
        Metrics.PushRate(FullOperationName, "error", "ydb");
        LogContext.LogEventErrorCombo<NEvClass::TMatrixYdbOperationError>(StorageName, OperationName, checkResult.Error());
        RtLogActivation->Finish(/* ok = */ false, checkResult.Error());
    }

    return checkResult;
}

} // namespace NMatrix

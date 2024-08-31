#pragma once

#include <alice/matrix/library/config/config.pb.h>
#include <alice/matrix/library/logging/log_context.h>
#include <alice/matrix/library/metrics/metrics.h>

#include <infra/libs/outcome/result.h>

#include <ydb/public/sdk/cpp/client/ydb_table/table.h>
#include <ydb/public/sdk/cpp/client/ydb_scheme/scheme.h>

#include <util/generic/noncopyable.h>


namespace NMatrix {

/* Base class for ydb storages.
 * Please implement all operations using 'ReportOperationResult' function.
 */
class IYDBStorage : public TMoveOnly {
public:
    IYDBStorage(
        const NYdb::TDriver& driver,
        const TYDBClientSettings& config,
        const TStringBuf name
    );

    virtual ~IYDBStorage() = default;

protected:
    using TResultChecker = std::function<TExpected<void, TString>(const NYdb::TStatus& res)>;
    static TExpected<void, TString> DefaultResultChecker(const NYdb::TStatus& res);

    struct TOperationSettings {
        bool ReportTimings;
    };

    struct TOperationContext {
        TOperationContext(
            const TString& storageName,
            const TString& operationName,
            const TOperationSettings& operationSettings,
            TLogContext logContext,
            TSourceMetrics& metrics
        );

        // All refs are intended here
        const TString& StorageName;
        const TString& OperationName;
        const TOperationSettings& OperationSettings;
        const TString FullOperationName;
        TLogContext LogContext;
        TSourceMetrics& Metrics;

        const TInstant StartTime;
        TAtomicSharedPtr<TRtLogActivation> RtLogActivation;

        TExpected<void, TString> ReportResult(
            const NYdb::TStatus& res,
            TResultChecker resultChecker = IYDBStorage::DefaultResultChecker
        );

        template <typename T>
        TExpected<void, TString> ParseProtoFromStringWithErrorReport(
            T& msg,
            const TString& data
        ) {
            const auto sensor = TString::Join(FullOperationName, "_proto_parse");
            if (msg.ParseFromString(data)) {
                Metrics.PushRate(sensor, "ok", "ydb");
                return TExpected<void, TString>::DefaultSuccess();
            }

            Metrics.PushRate(sensor, "error", "ydb");
            static const auto protoName = msg.GetDescriptor()->full_name();

            const TString err = TString::Join("Unable to parse proto '", protoName, '\'');
            LogContext.LogEventErrorCombo<NEvClass::TMatrixProtoParseError>(
                protoName,
                "", /* ErrorMessage */
                false, /* FromJson */
                data
            );
            RtLogActivation->Finish(/* ok = */ false, err);

            return err;
        }
    };

    static TString YDBStatusToString(const NYdb::TStatus& res);

protected:
    const TString Name_;

    NYdb::NTable::TTableClient Client_;
    const NYdb::NTable::TRetryOperationSettings RetryOperationSettings_;
    const NYdb::NTable::TExecDataQuerySettings ExecDataQuerySettings_;

    const TOperationSettings OperationSettings_;
};

} // namespace NMatrix

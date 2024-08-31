#include "context.h"
#include "session.h"
#include "ydb.h"

#include <alice/joker/library/log/log.h>

#include <library/cpp/unicode/normalization/normalization.h>

#include <util/system/env.h>

using namespace NYdb;

namespace NAlice::NJokerLight {

namespace {

inline constexpr TStringBuf UPSERT_SESSION_QUERY = TStringBuf(R"(
    PRAGMA TablePathPrefix("%s");
    UPSERT INTO sessions (id, fetch_if_not_exists, imitate_delay, dont_save) VALUES
        ("%s", %s, %s, %s);
)");

inline constexpr TStringBuf SELECT_SESSION_QUERY = TStringBuf(R"(
    PRAGMA TablePathPrefix("%s");
    SELECT * FROM sessions
    WHERE id = "%s";
)");

inline constexpr TStringBuf SELECT_STUB_QUERY = TStringBuf(R"(
    PRAGMA TablePathPrefix("%s");
    SELECT * FROM `%s`
    WHERE req_id = "%s" AND hash = "%s";
)");

inline constexpr TStringBuf UPSERT_STUB_ERROR_QUERY = TStringBuf(R"(
    PRAGMA TablePathPrefix("%s");
    UPSERT INTO `%s` (req_id, hash, error) VALUES
        ("%s", "%s", "%s");
)");

inline constexpr TStringBuf UPSERT_STUB_QUERY = TStringBuf(R"(
    PRAGMA TablePathPrefix("%s");

    DECLARE $data as String;

    UPSERT INTO `%s` (req_id, hash, created_at, duration_ms, url, data) VALUES
        ("%s", "%s", CurrentUtcTimestamp(), %s, "%s", $data);
)");

inline const char* BoolToStr(bool b) {
    return b ? "true" : "false";
}

void LogOnError(const NYdb::TStatus& status) {
    if (!status.IsSuccess()) {
        TStringStream ss;
        status.GetIssues().PrintTo(ss);
        LOG(ERROR) << "YDB error: " << ss.Str() << Endl;
    }
}

TDriver ConstructDriver(const TConfig& config) {
    auto driverConfig = TDriverConfig{}
        .SetEndpoint(TString{config.Endpoint()})
        .SetDatabase(TString{config.Database()})
        .SetAuthToken(GetEnv("YDB_TOKEN"));

    return TDriver{driverConfig};
}

NTable::TTableDescription BuildSessionTableDesc() {
    return NTable::TTableBuilder()
        .AddNullableColumn("req_id", EPrimitiveType::String)
        .AddNullableColumn("hash", EPrimitiveType::String)
        .AddNullableColumn("created_at", EPrimitiveType::Timestamp)
        .AddNullableColumn("duration_ms", EPrimitiveType::Uint64)
        .AddNullableColumn("url", EPrimitiveType::String)
        .AddNullableColumn("data", EPrimitiveType::String)
        .AddNullableColumn("error", EPrimitiveType::String)
        .SetPrimaryKeyColumns({"req_id", "hash"})
        .Build();
}

} // namespace

TYdb::TYdb(const TContext& context)
    : Context_{context}
    , Database_{Context_.Config().Database()}
    , Driver_{ConstructDriver(Context_.Config())}
    , Client_{Driver_}
{
}

void TYdb::UpsertSession(const TString& sessionId, const TSession::TSettings& settings) {
    LOG(INFO) << "Upserting info about session " << sessionId.Quote() << Endl;
    LogOnError(Client_.RetryOperationSync([&](NTable::TSession ydbSession) {
        auto query = Sprintf(UPSERT_SESSION_QUERY.begin(), Database_.c_str(), sessionId.c_str(),
                BoolToStr(settings.FetchIfNotExists), BoolToStr(settings.ImitateDelay), BoolToStr(settings.DontSave));
        auto txControl = NTable::TTxControl::BeginTx(NTable::TTxSettings::SerializableRW()).CommitTx();
        return ydbSession.ExecuteDataQuery(query, txControl).GetValueSync();
    }));

    LOG(INFO) << "Creating table for session " << sessionId.Quote() << Endl;
    LogOnError(Client_.RetryOperationSync([&](NTable::TSession ydbSession) {
        return ydbSession.CreateTable(TString::Join(Database_, "/", sessionId), BuildSessionTableDesc()).GetValueSync();
    }));
}

TMaybe<TSession::TSettings> TYdb::ObtainSession(const TString& sessionId) {
    LOG(INFO) << "Obtain session " << sessionId.Quote() << Endl;

    // Try to find in local cache
    auto cacheIter = SessionSettingsCache_.find(sessionId);
    if (cacheIter != SessionSettingsCache_.end()) {
        auto loadTimeIter = SessionSettingsLoadTime_.find(sessionId);
        TInstant loadTime = loadTimeIter->second;

        if (TInstant::Now() - loadTime <= TDuration::Minutes(5)) {
            return cacheIter->second;
        }

        SessionSettingsCache_.erase(cacheIter);
        SessionSettingsLoadTime_.erase(loadTimeIter);
    }

    LOG(INFO) << "Old local cache, load session " << sessionId.Quote() << " from YDB" << Endl;

    TMaybe<NYdb::TResultSet> resultSet;

    LogOnError(Client_.RetryOperationSync([&](NTable::TSession ydbSession) {
        auto query = Sprintf(SELECT_SESSION_QUERY.begin(), Database_.c_str(), sessionId.c_str());
        auto txControl = NTable::TTxControl::BeginTx(NTable::TTxSettings::SerializableRW()).CommitTx();
        auto result = ydbSession.ExecuteDataQuery(query, txControl).GetValueSync();
        if (result.IsSuccess()) {
            resultSet = result.GetResultSet(0);
        }
        return result;
    }));

    if (resultSet.Defined()) {
        TResultSetParser parser(resultSet.GetRef());
        if (parser.TryNextRow()) {
            LOG(INFO) << "Session " << sessionId.Quote() << " settings found" << Endl;

            TSession::TSettings settings;
            settings.FetchIfNotExists = parser.ColumnParser("fetch_if_not_exists").GetOptionalBool().GetRef();
            settings.ImitateDelay = parser.ColumnParser("imitate_delay").GetOptionalBool().GetRef();
            settings.DontSave = parser.ColumnParser("dont_save").GetOptionalBool().GetRef();

            SessionSettingsCache_.emplace(sessionId, settings);
            SessionSettingsLoadTime_.emplace(sessionId, TInstant::Now());

            return settings;
        }
    }

    return Nothing();
}

TMaybe<TResultSetParser> TYdb::ObtainStub(const TStubId& stubId) {
    LOG(INFO) << "Try to obtain stub of request " << stubId.ReqId.Quote() << " with hash " << stubId.Hash.Quote()
        << ", session id " << stubId.SessionId.Quote() << Endl;
    TMaybe<NYdb::TResultSet> resultSet;

    LogOnError(Client_.RetryOperationSync([&](NTable::TSession ydbSession) {
        auto query = Sprintf(SELECT_STUB_QUERY.begin(), Database_.c_str(), stubId.SessionId.c_str(), stubId.ReqId.c_str(), stubId.Hash.c_str());
        auto txControl = NTable::TTxControl::BeginTx(NTable::TTxSettings::SerializableRW()).CommitTx();
        auto result = ydbSession.ExecuteDataQuery(query, txControl).GetValueSync();
        if (result.IsSuccess()) {
            resultSet = result.GetResultSet(0);
        }
        return result;
    }));

    if (resultSet.Defined()) {
        TResultSetParser parser(resultSet.GetRef());
        if (!parser.TryNextRow()) {
            return Nothing();
        }
        return parser;
    }

    return Nothing();
}

void TYdb::SaveStubError(const TStubId& stubId, const TError& error) {
    auto errorStr = error.AsString();

    Client_.RetryOperation([this, stubId, errorStr](NTable::TSession ydbSession) -> NYdb::TAsyncStatus {
        LOG(INFO) << "Saving stub error: Execute data query " << stubId.ReqId.Quote() << ", " << errorStr.Quote() << Endl;

        auto query = Sprintf(UPSERT_STUB_ERROR_QUERY.begin(), Database_.c_str(), stubId.SessionId.c_str(),
            stubId.ReqId.c_str(), stubId.Hash.c_str(), errorStr.c_str());

        return ydbSession.ExecuteDataQuery(
            query,
            NTable::TTxControl::BeginTx(NTable::TTxSettings::SerializableRW()).CommitTx()
        ).Apply([](const NTable::TAsyncDataQueryResult& handle) {
            NTable::TDataQueryResult result = handle.GetValue();
            return NThreading::MakeFuture<NYdb::TStatus>(result);
        });
    }).Subscribe([stubId](NYdb::TAsyncStatus status) {
        LOG(INFO) << "Saved stub error for " << stubId.ReqId.Quote() << ", " << stubId.Hash.Quote() << Endl;
        LogOnError(status.GetValue());
    });
}

void TYdb::SaveStub(const TStubId& stubId, NNeh::TResponseRef response, const TString& data, const TString& addr) {
    Client_.RetryOperation([this, stubId, response, data, addr](NTable::TSession ydbSession) -> NYdb::TAsyncStatus {
        LOG(INFO) << "Saving stub: Prepare data query " << stubId.ReqId.Quote() << ", " << addr.Quote() << Endl;

        TString durationStr = ToString(response->Duration.MilliSeconds());
        auto query = Sprintf(UPSERT_STUB_QUERY.begin(), Database_.c_str(), stubId.SessionId.c_str(),
            stubId.ReqId.c_str(), stubId.Hash.c_str(), durationStr.c_str(),
            addr.c_str());

        return ydbSession.PrepareDataQuery(query)
            .Apply([data, stubId, addr](const NTable::TAsyncPrepareQueryResult& result) -> NYdb::TAsyncStatus {
                LOG(INFO) << "Saving stub: Setting params " << stubId.ReqId.Quote() << ", " << addr.Quote() << Endl;
                auto resultValue = result.GetValueSync();
                if (!resultValue.IsSuccess()) {
                    return NThreading::MakeFuture<NYdb::TStatus>(resultValue);
                }

                auto dataQuery = resultValue.GetQuery();
                auto params = dataQuery.GetParamsBuilder()
                    .AddParam("$data")
                        .String(data)
                        .Build()
                    .Build();
                return dataQuery.Execute(NTable::TTxControl::BeginTx(NTable::TTxSettings::SerializableRW()).CommitTx(),
                    std::move(params)).Apply([](const NThreading::TFuture<NTable::TDataQueryResult>& queryResult) {
                        return NThreading::MakeFuture<NYdb::TStatus>(queryResult.GetValueSync());
                    });
            });
    }).Subscribe([stubId, addr](NYdb::TAsyncStatus status) {
        LOG(INFO) << "Saved stub for " << stubId.ReqId.Quote() << ", " << addr.Quote() << Endl;
        LogOnError(status.GetValue());
    });
}

} // namespace NAlice::NJokerLight

#include "long_session_client.h"

#include <alice/bass/libs/ydb_helpers/exception.h>

#include <util/string/printf.h>

namespace NAlice::NHollywood::NGeneralConversation {

using namespace NYdb;

namespace {

const auto YDB_RETRY = NYdb::NTable::TRetryOperationSettings()
    .GetSessionClientTimeout(TDuration::MilliSeconds(50))
    .MaxRetries(3);

static TStatus SelectLongSession(NTable::TSession session, const TString& path, const TString& uuid, TMaybe<TResultSet>& resultSet) {
    auto query = Sprintf(R"(
        PRAGMA TablePathPrefix("%s");

        DECLARE $uuid AS String;

        SELECT uuid, long_session
        FROM LongSession
        WHERE uuid = $uuid;
    )", path.c_str());
    auto prepareResult = session.PrepareDataQuery(query).GetValueSync();
    if (!prepareResult.IsSuccess()) {
        return prepareResult;
    }

    auto preparedQuery = prepareResult.GetQuery();
    auto params = preparedQuery.GetParamsBuilder()
        .AddParam("$uuid")
            .String(uuid)
            .Build()
        .Build();
    auto txControl = NTable::TTxControl::BeginTx(NTable::TTxSettings::SerializableRW()).CommitTx();

    auto result = preparedQuery.Execute(txControl, params).GetValueSync();
    if (result.IsSuccess()) {
        resultSet = result.GetResultSet(0);
    }

    return result;
}

static TStatus UpdateLongSession(NTable::TSession session, const TString& path, const TString& uuid, const TLongSession& longSession, ui64 serverTimeMs) {
    auto query = Sprintf(R"(
        PRAGMA TablePathPrefix("%s");

        DECLARE $uuid AS String;
        DECLARE $long_session AS String;
        DECLARE $server_time_ms AS uint64;

        UPSERT INTO LongSession (uuid, long_session, server_time_ms) VALUES
            ($uuid, $long_session, $server_time_ms);
    )", path.c_str());
    auto prepareResult = session.PrepareDataQuery(query).GetValueSync();
    if (!prepareResult.IsSuccess()) {
        return prepareResult;
    }

    auto preparedQuery = prepareResult.GetQuery();
    TString message;
    Y_PROTOBUF_SUPPRESS_NODISCARD longSession.SerializeToString(&message);
    auto params = preparedQuery.GetParamsBuilder()
        .AddParam("$uuid")
            .String(uuid)
            .Build()
        .AddParam("$long_session")
            .String(message)
            .Build()
        .AddParam("$server_time_ms")
            .Uint64(serverTimeMs)
            .Build()
        .Build();
    auto txControl = NTable::TTxControl::BeginTx(NTable::TTxSettings::SerializableRW()).CommitTx();

    return preparedQuery.Execute(txControl, params).GetValueSync();
}

std::unique_ptr<TDriver> ConstructDriver(const TString& endpoint, const TString& database, const TString& authToken) {
    const TDriverConfig ydbConfig = TDriverConfig()
        .SetEndpoint(endpoint)
        .SetDatabase(database)
        .SetAuthToken(authToken);

    return std::make_unique<TDriver>(ydbConfig);
}

} // namespace

TLongSessionClient::TLongSessionClient(const TParams& params)
    : Driver(ConstructDriver(params.Endpoint, params.Database, params.AuthToken))
    , Client(std::make_unique<NYdb::NTable::TTableClient>(*Driver.get()))
    , Path(params.Path)
{
    // Warm up
    TMaybe<TResultSet> resultSet;
    Client->RetryOperationSync([&path=Path, &resultSet](NTable::TSession session) {
        return SelectLongSession(session, path, "", resultSet);
    }, YDB_RETRY);
}

TLongSession TLongSessionClient::Get(const TString& uuid) {
    TMaybe<TResultSet> resultSet;
    const auto status = Client->RetryOperationSync([&path=Path, &uuid, &resultSet](NTable::TSession session) {
        return SelectLongSession(session, path, uuid, resultSet);
    }, YDB_RETRY);

    NYdbHelpers::ThrowOnError(status);

    TLongSession longSession;
    TResultSetParser parser(resultSet.GetRef());
    while (parser.TryNextRow()) {
        auto value = parser.ColumnParser("long_session").GetOptionalString();
        if (value) {
            Y_PROTOBUF_SUPPRESS_NODISCARD longSession.ParseFromString(value.GetRef());
        }
    }

    return longSession;
}

void TLongSessionClient::Update(const TString& uuid, const TLongSession& longSession, ui64 serverTimeMs) {
    const auto status = Client->RetryOperationSync([&path=Path, &uuid, &longSession, &serverTimeMs](NTable::TSession session) {
        return UpdateLongSession(session, path, uuid, longSession, serverTimeMs);
    }, YDB_RETRY);

    NYdbHelpers::ThrowOnError(status);
}

void TLongSessionClient::Shutdown() {
    if (Client) {
        Client->Stop().Wait();
    }
    if (Driver) {
        Driver->Stop(true);
    }
    Client.reset();
    Driver.reset();
}

} // namespace NAlice::NHollywood::NGeneralConversation

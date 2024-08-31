#include "ydb.h"

#include <alice/joker/library/log/log.h>

#include <util/stream/str.h>
#include <util/string/printf.h>

using namespace NYdb;

namespace NAlice::NShooter {

namespace {

void ThrowOnError(const NYdb::TStatus& status) {
    if (!status.IsSuccess()) {
        TStringStream ss;
        status.GetIssues().PrintTo(ss);
        ythrow yexception() << "YDB error: " << ss.Str();
    }
}

TDriver ConstructDriver(const TString& endpoint, const TString& database, const TString& authToken) {
    auto config = TDriverConfig{}
        .SetEndpoint(endpoint)
        .SetDatabase(database)
        .SetAuthToken(authToken);

    return TDriver{config};
}

} // namespace

TYdb::TYdb(const TString& endpoint, const TString& database, const TString& authToken)
    : Endpoint_{endpoint}
    , Database_{database}
    , AuthToken_{authToken}
    , Driver_{ConstructDriver(Endpoint_, Database_, AuthToken_)}
{
}

TYdb::~TYdb() {
    Driver_.Stop(/* wait = */ true);
}

TString TYdb::ObtainConfig(const TString& table, const TString& id) {
    LOG(INFO) << "YDB settings: endpoint " << Endpoint_.Quote() << ", database " << Database_.Quote() << Endl;
    LOG(INFO) << "Get config by table " << table.Quote() << ", id " << id.Quote() << Endl;

    NTable::TTableClient client{Driver_};
    TMaybe<NYdb::TResultSet> resultSet;

    ThrowOnError(client.RetryOperationSync([&](NYdb::NTable::TSession session) {
        auto query = Sprintf(R"(
            PRAGMA TablePathPrefix("%s");
            SELECT config FROM %s WHERE id = "%s";
        )", Database_.c_str(), table.c_str(), id.c_str());

        auto txControl =
            // Begin new transaction with SerializableRW mode
            NTable::TTxControl::BeginTx(NTable::TTxSettings::SerializableRW())
            // Commit transaction at the end of the query
            .CommitTx();

        auto result = session.ExecuteDataQuery(query, txControl).GetValueSync();
        if (result.IsSuccess()) {
            resultSet = result.GetResultSet(0);
        }
        return result;
    }));

    NYdb::TResultSetParser parser(resultSet.GetRef());
    Y_ASSERT(parser.TryNextRow());
    return parser.ColumnParser("config").GetOptionalString().GetRef();
}

} // namespace NAlice::NShooter

#include "long_session_client.h"

#include <alice/bass/libs/ydb_helpers/exception.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/folder/pathsplit.h>
#include <util/system/env.h>

using namespace NYdb;
using namespace NAlice::NHollywood::NGeneralConversation;

static TString JoinPath(const TString& basePath, const TString& path) {
    if (basePath.empty()) {
        return path;
    }

    TPathSplitUnix prefixPathSplit(basePath);
    prefixPathSplit.AppendComponent(path);

    return prefixPathSplit.Reconstruct();
}

void CreateTables(const TString& path) {
    const TDriverConfig ydbConfig = TDriverConfig()
        .SetEndpoint(GetEnv("YDB_ENDPOINT"))
        .SetDatabase(GetEnv("YDB_DATABASE"))
        .SetAuthToken(GetEnv("YDB_TOKEN"));

    NYdb::NTable::TTableClient client(TDriver{ydbConfig});

    const auto status = client.RetryOperationSync([&path](NTable::TSession session) {
        auto seriesDesc = NTable::TTableBuilder()
            .AddNullableColumn("uuid", EPrimitiveType::String)
            .AddNullableColumn("long_session", EPrimitiveType::String)
            .AddNullableColumn("server_time_ms", EPrimitiveType::Uint64)
            .SetPrimaryKeyColumn("uuid")
            .Build();

        return session.CreateTable(JoinPath(path, "LongSession"), std::move(seriesDesc)).GetValueSync();
    });

    NYdbHelpers::ThrowOnError(status);
}

Y_UNIT_TEST_SUITE_F(LongSession, NUnitTest::TBaseFixture) {
    Y_UNIT_TEST(None) {
        TString path = GetEnv("YDB_DATABASE");
        TString uuid = "1";
        CreateTables(path);
        TLongSessionClient client({GetEnv("YDB_ENDPOINT"), GetEnv("YDB_DATABASE"), path, ""});
        auto longSession = client.Get(uuid);
        UNIT_ASSERT_EQUAL(longSession.GetProactivityLastSuggestServerTimeMs(), 0);
    }

    Y_UNIT_TEST(Create) {
        TString path = GetEnv("YDB_DATABASE");
        TString uuid = "2";
        CreateTables(path);
        TLongSessionClient client({GetEnv("YDB_ENDPOINT"), GetEnv("YDB_DATABASE"), path, ""});
        auto longSession = client.Get(uuid);
        UNIT_ASSERT_EQUAL(longSession.GetProactivityLastSuggestServerTimeMs(), 0);

        longSession.SetProactivityLastSuggestServerTimeMs(1);
        client.Update(uuid, longSession, 1);

        auto longSessionUpdated = client.Get(uuid);
        UNIT_ASSERT_EQUAL(longSession.GetProactivityLastSuggestServerTimeMs(), longSessionUpdated.GetProactivityLastSuggestServerTimeMs());
    }

    Y_UNIT_TEST(Update) {
        TString path = GetEnv("YDB_DATABASE");
        TString uuid = "3";
        CreateTables(path);
        TLongSessionClient client({GetEnv("YDB_ENDPOINT"), GetEnv("YDB_DATABASE"), path, ""});
        auto longSession = client.Get(uuid);
        UNIT_ASSERT_EQUAL(longSession.GetProactivityLastSuggestServerTimeMs(), 0);

        longSession.SetProactivityLastSuggestServerTimeMs(1);
        client.Update(uuid, longSession, 1);

        auto longSessionUpdated = client.Get(uuid);
        UNIT_ASSERT_EQUAL(longSession.GetProactivityLastSuggestServerTimeMs(), longSessionUpdated.GetProactivityLastSuggestServerTimeMs());

        longSession.SetProactivityLastSuggestServerTimeMs(2);
        client.Update(uuid, longSession, 1);

        longSessionUpdated = client.Get(uuid);
        UNIT_ASSERT_EQUAL(longSession.GetProactivityLastSuggestServerTimeMs(), longSessionUpdated.GetProactivityLastSuggestServerTimeMs());

    }

    Y_UNIT_TEST(Update2Clients) {
        TString path = GetEnv("YDB_DATABASE");
        TString uuid = "4";
        CreateTables(path);
        TLongSessionClient clientGet({GetEnv("YDB_ENDPOINT"), GetEnv("YDB_DATABASE"), path, ""});
        TLongSessionClient clientUpdate({GetEnv("YDB_ENDPOINT"), GetEnv("YDB_DATABASE"), path, ""});
        auto longSession = clientGet.Get(uuid);
        UNIT_ASSERT_EQUAL(longSession.GetProactivityLastSuggestServerTimeMs(), 0);

        longSession.SetProactivityLastSuggestServerTimeMs(1);
        clientUpdate.Update(uuid, longSession, 1);

        auto longSessionUpdated = clientGet.Get(uuid);
        UNIT_ASSERT_EQUAL(longSession.GetProactivityLastSuggestServerTimeMs(), longSessionUpdated.GetProactivityLastSuggestServerTimeMs());

        longSession.SetProactivityLastSuggestServerTimeMs(2);
        clientUpdate.Update(uuid, longSession, 1);

        longSessionUpdated = clientGet.Get(uuid);
        UNIT_ASSERT_EQUAL(longSession.GetProactivityLastSuggestServerTimeMs(), longSessionUpdated.GetProactivityLastSuggestServerTimeMs());

    }

}

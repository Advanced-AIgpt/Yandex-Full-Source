#include <alice/wonderlogs/library/yt/utils.h>

#include <alice/wonderlogs/library/common/utils.h>

#include <library/cpp/testing/unittest/registar.h>

#include <mapreduce/yt/tests/yt_unittest_lib/yt_unittest_lib.h>

using namespace NAlice::NWonderlogs;

Y_UNIT_TEST_SUITE(Utils) {
    Y_UNIT_TEST(CreateTableWithoutExpirationDate) {
        auto client = NYT::NTesting::CreateTestClient();
        const auto directory = NYT::NTesting::CreateTestDirectory(client);

        const auto table = directory + "/abc";
        CreateTable(client, table, /* expirationDate= */ {});
        UNIT_ASSERT(client->Exists(table));
    }

    Y_UNIT_TEST(CreateTableWithExpirationDate) {
        auto client = NYT::NTesting::CreateTestClient();
        const auto directory = NYT::NTesting::CreateTestDirectory(client);

        const auto table = directory + "/abc";
        const auto expirationDate = TInstant::Seconds(TInstant::Now().Seconds()) + TDuration::Parse("30d");
        CreateTable(client, table, expirationDate);
        UNIT_ASSERT(client->Exists(table));
        const auto qwe = client->Get(table + "/@expiration_time").AsString();
        UNIT_ASSERT_EQUAL(expirationDate.ToString(), qwe);
    }

    Y_UNIT_TEST(CreateRandomTable) {
        auto client = NYT::NTesting::CreateTestClient();
        const auto directory = NYT::NTesting::CreateTestDirectory(client);
        const auto table = CreateRandomTable(client, directory, "qwe");

        const auto expirationTime = ParseDatetime(client->Get(table + "/@expiration_time").AsString());
        UNIT_ASSERT(expirationTime);
        UNIT_ASSERT(expirationTime->MilliSeconds() > (TInstant::Now() + TDuration::Parse("6d")).MilliSeconds());
        UNIT_ASSERT(expirationTime->MilliSeconds() < (TInstant::Now() + TDuration::Parse("8d")).MilliSeconds());
    }
}

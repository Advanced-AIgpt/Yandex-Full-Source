#include <alice/matrix/notificator/library/storages/connections/storage.h>

#include <library/cpp/testing/gtest/gtest.h>


TEST(TMatrixNotificatorConnectionsStorageEndpointTest, TestGetTableShardKey) {
    NMatrix::NNotificator::TConnectionsStorage::TEndpoint endpoint = {
        .Ip = "",
        .ShardId = 0,
        .Port = 0,
        .Monotonic = 0,
    };
    EXPECT_EQ(endpoint.GetTableShardKey(), 17354628760588346902ull);

    endpoint.Ip = "128.0.0.1";
    EXPECT_EQ(endpoint.GetTableShardKey(), 12543175492853562073ull);

    endpoint.ShardId = 1;
    EXPECT_EQ(endpoint.GetTableShardKey(), 16437609986777225945ull);
}

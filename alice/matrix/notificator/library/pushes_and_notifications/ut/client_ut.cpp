#include <alice/matrix/notificator/library/pushes_and_notifications/client.h>

#include <library/cpp/testing/gtest/gtest.h>

TEST(TMatrixNotificatorPushesAndNotificationsClientTest, TestUserSubscriptionsInfo) {
    NMatrix::NNotificator::TPushesAndNotificationsClient::TUserSubscriptionsInfo userSubscriptionsInfo = {
        .SubscriptionIdToUserSubscriptionInfo = {
            {
                1,
                NMatrix::NNotificator::TSubscriptionsStorage::TUserSubscription({
                    .SubscribedAtTimestamp = 42,
                }),
            },
        },
        .UnsubscribedDevices = {
            "device_id1",
            "device_id2",
        },
    };

    {
        // IsUserSubscribedToSubscription
        EXPECT_FALSE(userSubscriptionsInfo.IsUserSubscribedToSubscription(0));
        EXPECT_TRUE(userSubscriptionsInfo.IsUserSubscribedToSubscription(1));
        EXPECT_FALSE(userSubscriptionsInfo.IsUserSubscribedToSubscription(2));
    }

    {
        // IsDeviceSubscribed
        EXPECT_FALSE(userSubscriptionsInfo.IsDeviceSubscribed("device_id1"));
        EXPECT_FALSE(userSubscriptionsInfo.IsDeviceSubscribed("device_id2"));
        EXPECT_TRUE(userSubscriptionsInfo.IsDeviceSubscribed("device_id3"));
    }
}


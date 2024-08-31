#include <alice/matrix/notificator/library/subscriptions_info/subscriptions_info.h>

#include <library/cpp/testing/gtest/gtest.h>


TEST(TMatrixNotificatorSubscriptionsInfoTest, TestGetCategories) {
    const auto& subscriptionsInfo = NMatrix::NNotificator::GetSubscriptionsInfo();

    const auto& categories = subscriptionsInfo.GetCategories();

    ASSERT_GE(categories.size(), 1u);

    const auto& firstCategory = categories[0];
    EXPECT_EQ(firstCategory.GetId(), 1u);
    EXPECT_EQ(firstCategory.GetName(), "Общие");
}

TEST(TMatrixNotificatorSubscriptionsInfoTest, TestGetSubscriptions) {
    const auto& subscriptionsInfo = NMatrix::NNotificator::GetSubscriptionsInfo();

    const auto& subscriptions = subscriptionsInfo.GetSubscriptions();

    // Just simple checks
    {
        const auto subscription = subscriptions.FindPtr(1);
        ASSERT_NE(subscription, nullptr);

        EXPECT_EQ(subscription->GetName(), "Дайджест Алисы");
        EXPECT_EQ(subscription->GetType(), NMatrix::NNotificator::TSubscription::USER);
    }

    {
        const auto subscription = subscriptions.FindPtr(6);
        ASSERT_NE(subscription, nullptr);

        EXPECT_EQ(subscription->GetName(), "Свежий подкаст");
        EXPECT_EQ(subscription->GetType(), NMatrix::NNotificator::TSubscription::USER);
    }
}

TEST(TMatrixNotificatorSubscriptionsInfoTest, TestHasSubscription) {
    const auto& subscriptionsInfo = NMatrix::NNotificator::GetSubscriptionsInfo();

    EXPECT_FALSE(subscriptionsInfo.HasSubscription(0));
    EXPECT_TRUE(subscriptionsInfo.HasSubscription(1));
    EXPECT_TRUE(subscriptionsInfo.HasSubscription(6));
    EXPECT_FALSE(subscriptionsInfo.HasSubscription(100500));

    for (const auto& [subscriptionId, subscriptionInfo] : subscriptionsInfo.GetSubscriptions()) {
        EXPECT_TRUE(subscriptionsInfo.HasSubscription(subscriptionId));
    }
}

TEST(TMatrixNotificatorSubscriptionsInfoTest, TestIsUserDeviceTypeSuitableForSubscriptions) {
    const auto& subscriptionsInfo = NMatrix::NNotificator::GetSubscriptionsInfo();

    EXPECT_FALSE(subscriptionsInfo.IsUserDeviceTypeSuitableForSubscriptions(NAlice::UnknownDeviceType));
    EXPECT_FALSE(subscriptionsInfo.IsUserDeviceTypeSuitableForSubscriptions(NAlice::LightDeviceType));
    EXPECT_TRUE(subscriptionsInfo.IsUserDeviceTypeSuitableForSubscriptions(NAlice::JBLLinkMusicDeviceType));
    EXPECT_TRUE(subscriptionsInfo.IsUserDeviceTypeSuitableForSubscriptions(NAlice::JBLLinkPortableDeviceType));
    EXPECT_TRUE(subscriptionsInfo.IsUserDeviceTypeSuitableForSubscriptions(NAlice::YandexStationDeviceType));
    EXPECT_TRUE(subscriptionsInfo.IsUserDeviceTypeSuitableForSubscriptions(NAlice::YandexStation2DeviceType));
    EXPECT_FALSE(subscriptionsInfo.IsUserDeviceTypeSuitableForSubscriptions(NAlice::YandexStationChironDeviceType));
}

TEST(TMatrixNotificatorSubscriptionsInfoTest, TestIsDeviceModelSuitableForSubscription) {
    const auto& subscriptionsInfo = NMatrix::NNotificator::GetSubscriptionsInfo();

    {
        EXPECT_TRUE(subscriptionsInfo.IsDeviceModelSuitableForSubscription(1, "yandexstation"));
        EXPECT_TRUE(subscriptionsInfo.IsDeviceModelSuitableForSubscription(1, "station_2"));
        EXPECT_TRUE(subscriptionsInfo.IsDeviceModelSuitableForSubscription(1, "yandexmini_2_no_clock"));

        EXPECT_TRUE(subscriptionsInfo.IsDeviceModelSuitableForSubscription(2, "station_2"));
        EXPECT_FALSE(subscriptionsInfo.IsDeviceModelSuitableForSubscription(2, "yandexmini_2_no_clock"));
        EXPECT_FALSE(subscriptionsInfo.IsDeviceModelSuitableForSubscription(2, "qwerty"));
    }

    {
        // Special cases

        // No subscription
        EXPECT_FALSE(subscriptionsInfo.IsDeviceModelSuitableForSubscription(12345, "station_2"));
        // station == yandexstation hack
        EXPECT_TRUE(subscriptionsInfo.IsDeviceModelSuitableForSubscription(1, "station"));
        // Empty device id
        EXPECT_TRUE(subscriptionsInfo.IsDeviceModelSuitableForSubscription(1, ""));
        // Empty device models filter (one more legacy hack) (4 is a technical subscription)
        EXPECT_TRUE(subscriptionsInfo.IsDeviceModelSuitableForSubscription(4, "any_string"));
    }
}

TEST(TMatrixNotificatorSubscriptionsInfoTest, FilterListConnectionsResultBySubscriptionAndUnsubscribedDevices) {
    const auto& subscriptionsInfo = NMatrix::NNotificator::GetSubscriptionsInfo();

    static auto createEndpoint = [](const TString& ip) {
        return NMatrix::NNotificator::TConnectionsStorage::TEndpoint({
            .Ip = ip,
            .ShardId = 0,
            .Port = 10,
            .Monotonic = 0,
        });
    };

    static auto createDeviceInfo = [](const TString& deviceModel) {
        NMatrix::NNotificator::TDeviceInfo result;
        result.SetDeviceModel(deviceModel);
        return result;
    };

    static auto createUserDeviceInfo = [](const TString& deviceId, const TString& deviceModel) {
        return NMatrix::NNotificator::TConnectionsStorage::TUserDeviceInfo({
            .Puid = "puid",
            .DeviceId =  deviceId,
            .DeviceInfo = createDeviceInfo(deviceModel),
        });
    };

    static auto createListConnectionsResultRecord = [](const TString& ip, const TString& deviceId, const TString& deviceModel) {
        return NMatrix::NNotificator::TConnectionsStorage::TListConnectionsResult::TRecord({
            .Endpoint = createEndpoint(ip),
            .UserDeviceInfo = createUserDeviceInfo(deviceId, deviceModel),
        });
    };

    static auto getDeviceIds = [](const NMatrix::NNotificator::TConnectionsStorage::TListConnectionsResult& listConnectionsResult) {
        TVector<TString> result;
        result.reserve(listConnectionsResult.Records.size());
        for (const auto& listConnectionsRecord : listConnectionsResult.Records) {
            result.push_back(listConnectionsRecord.UserDeviceInfo.DeviceId);
        }
        return result;
    };

    NMatrix::NNotificator::TConnectionsStorage::TListConnectionsResult listConnectionsResult = {
        .Records = {
            createListConnectionsResultRecord("ip1", "device_id1", "random"),
            createListConnectionsResultRecord("ip1", "device_id2", "yandexmini_2_no_clock"),
            createListConnectionsResultRecord("ip1", "device_id3", "yandexstation"),
            createListConnectionsResultRecord("ip1", "device_id4", "yandexstation"),
        },
    };

    EXPECT_THAT(
        getDeviceIds(
            subscriptionsInfo.FilterListConnectionsResultBySubscriptionAndUnsubscribedDevices(
                listConnectionsResult,
                1,
                {}
            )
        ),
        // No unknown devices
        testing::ContainerEq(TVector<TString>({"device_id2", "device_id3", "device_id4"}))
    );

    EXPECT_THAT(
        getDeviceIds(
            subscriptionsInfo.FilterListConnectionsResultBySubscriptionAndUnsubscribedDevices(
                listConnectionsResult,
                1,
                {"device_id3"}
            )
        ),
        testing::ContainerEq(TVector<TString>({"device_id2", "device_id4"}))
    );

    EXPECT_THAT(
        getDeviceIds(
            subscriptionsInfo.FilterListConnectionsResultBySubscriptionAndUnsubscribedDevices(
                listConnectionsResult,
                2,
                {}
            )
        ),
        // Filter device_id2 by subscription filter
        testing::ContainerEq(TVector<TString>({"device_id3", "device_id4"}))
    );

    EXPECT_THAT(
        getDeviceIds(
            subscriptionsInfo.FilterListConnectionsResultBySubscriptionAndUnsubscribedDevices(
                listConnectionsResult,
                12345,
                {}
            )
        ),
        // Ignore unknown subscription
        testing::ContainerEq(TVector<TString>({}))
    );
}

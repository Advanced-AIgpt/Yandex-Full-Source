#include <alice/matrix/notificator/library/storages/notifications/storage.h>

#include <library/cpp/testing/gtest/gtest.h>


TEST(TMatrixNotificatorNotificationsStorageTest, TestGetNotificationContentHash) {
    NMatrix::NNotificator::TNotificationsStorage::TNotification notification = {
        .DeviceId = "device_id",
        .Notification = {},
    };
    EXPECT_EQ(notification.GetNotificationContentHash(), 15003389587664935681ull);


    notification.Notification.SetSubscriptionId("2");
    notification.Notification.SetText("some_text");
    notification.Notification.SetVoice("some_voice");
    EXPECT_EQ(notification.GetNotificationContentHash(), 12521240089081050773ull);


    notification.Notification.SetSubscriptionId(notification.Notification.GetSubscriptionId() + "_other");
    EXPECT_EQ(notification.GetNotificationContentHash(), 796291451418492113ull);

    notification.Notification.SetText(notification.Notification.GetText() + "_other");
    EXPECT_EQ(notification.GetNotificationContentHash(), 2886587872790256083ull);

    notification.Notification.SetVoice(notification.Notification.GetVoice() + "_other");
    EXPECT_EQ(notification.GetNotificationContentHash(), 14510298008946095849ull);
}

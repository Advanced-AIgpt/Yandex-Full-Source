#include <alice/matrix/notificator/analytics/push_events_reducer/library/enum_value_ordering.h>

#include <library/cpp/testing/gtest/gtest.h>


TEST(TMatrixAnalyticsEnumValueOrderingTest, TestValid) {
    NMatrix::NAnalytics::NPrivate::ValidateEnumValueOrderingExtension(
        NMatrix::NAnalytics::GetProtoEnumDescriptorForAnalyticsValueOrdering<
            NEvClass::TMatrixNotificatorAnalyticsTechnicalPushValidationResult::EResult
        >()
    );
}

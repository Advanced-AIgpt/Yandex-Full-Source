#include <alice/matrix/analytics/enum_value_ordering/enum_value_ordering.h>
#include <alice/matrix/analytics/enum_value_ordering/ut/protos/test_enum_value_ordering.pb.h>

#include <library/cpp/testing/gtest/gtest.h>


namespace NMatrix::NAnalytics {

template <>
const NProtoBuf::EnumDescriptor* GetProtoEnumDescriptorForAnalyticsValueOrdering<
    NMatrix::NAnalytics::NTest::TMatrixAnalyticsValidEnumValueOrdering::EEnumType
>() {
    return NMatrix::NAnalytics::NTest::TMatrixAnalyticsValidEnumValueOrdering::EEnumType_descriptor();
}

} // namespace NMatrix::NAnalytics

TEST(TMatrixAnalyticsEnumValueOrderingTest, TestValid) {
    NMatrix::NAnalytics::NPrivate::ValidateEnumValueOrderingExtension(
        NMatrix::NAnalytics::NTest::TMatrixAnalyticsValidEnumValueOrdering::EEnumType_descriptor()
    );
}

TEST(TMatrixAnalyticsEnumValueOrderingTest, TestOutOfRangePriorities) {
    EXPECT_THROW_MESSAGE_HAS_SUBSTR(
        NMatrix::NAnalytics::NPrivate::ValidateEnumValueOrderingExtension(
            NMatrix::NAnalytics::NTest::TMatrixAnalyticsOutOfRangePrioritiesEnumValueOrdering::EEnumType_descriptor()
        ),
        yexception,
        "out of range"
    );
}

TEST(TMatrixAnalyticsEnumValueOrderingTest, TestDuplicatePriorities) {
    EXPECT_THROW_MESSAGE_HAS_SUBSTR(
        NMatrix::NAnalytics::NPrivate::ValidateEnumValueOrderingExtension(
            NMatrix::NAnalytics::NTest::TMatrixAnalyticsDuplicatePrioritiesEnumValueOrdering::EEnumType_descriptor()
        ),
        yexception,
        "Duplicate extension option"
    );
}

TEST(TMatrixAnalyticsEnumValueOrderingTest, TestMissingPriority) {
    EXPECT_THROW_MESSAGE_HAS_SUBSTR(
        NMatrix::NAnalytics::NPrivate::ValidateEnumValueOrderingExtension(
            NMatrix::NAnalytics::NTest::TMatrixAnalyticsMissingPriorityEnumValueOrdering::EEnumType_descriptor()
        ),
        yexception,
        "Missing extension option"
    );
}

TEST(TMatrixAnalyticsEnumValueOrderingTest, TestMinMaxEnumValueByAnalyticsPrioritiesEqualArguments) {
    using TEnumType = NMatrix::NAnalytics::NTest::TMatrixAnalyticsValidEnumValueOrdering;

    EXPECT_EQ(
        NMatrix::NAnalytics::MinEnumValueByAnalyticsPriority(
            TEnumType::VALUE_ONE_PRIORITY_ONE,
            TEnumType::VALUE_ONE_PRIORITY_ONE
        ),
        TEnumType::VALUE_ONE_PRIORITY_ONE
    );

    EXPECT_EQ(
        NMatrix::NAnalytics::MaxEnumValueByAnalyticsPriority(
            TEnumType::VALUE_ONE_PRIORITY_ONE,
            TEnumType::VALUE_ONE_PRIORITY_ONE
        ),
        TEnumType::VALUE_ONE_PRIORITY_ONE
    );
}

TEST(TMatrixAnalyticsEnumValueOrderingTest, TestMinMaxEnumValueByAnalyticsPrioritiesUnequalArguments) {
    using TEnumType = NMatrix::NAnalytics::NTest::TMatrixAnalyticsValidEnumValueOrdering;

    EXPECT_EQ(
        NMatrix::NAnalytics::MinEnumValueByAnalyticsPriority(
            TEnumType::VALUE_THREE_PRIORITY_FIVE,
            TEnumType::VALUE_FOUR_PRIORITY_FOUR
        ),
        TEnumType::VALUE_FOUR_PRIORITY_FOUR
    );

    EXPECT_EQ(
        NMatrix::NAnalytics::MinEnumValueByAnalyticsPriority(
            TEnumType::VALUE_FOUR_PRIORITY_FOUR,
            TEnumType::VALUE_THREE_PRIORITY_FIVE
        ),
        TEnumType::VALUE_FOUR_PRIORITY_FOUR
    );

    EXPECT_EQ(
        NMatrix::NAnalytics::MaxEnumValueByAnalyticsPriority(
            TEnumType::VALUE_THREE_PRIORITY_FIVE,
            TEnumType::VALUE_FOUR_PRIORITY_FOUR
        ),
        TEnumType::VALUE_THREE_PRIORITY_FIVE
    );

    EXPECT_EQ(
        NMatrix::NAnalytics::MaxEnumValueByAnalyticsPriority(
            TEnumType::VALUE_FOUR_PRIORITY_FOUR,
            TEnumType::VALUE_THREE_PRIORITY_FIVE
        ),
        TEnumType::VALUE_THREE_PRIORITY_FIVE
    );
}

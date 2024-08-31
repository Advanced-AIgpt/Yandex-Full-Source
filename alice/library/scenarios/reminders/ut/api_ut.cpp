#include "ut.h"

namespace {

using namespace NAlice::NRemindersApi;
using namespace NAlice::NRemindersApi::NTesting;

class TRemindersApiAppend: public TRemindersApiFixture,
                           public ::testing::WithParamInterface<TParametrizeAction>
{
};

class TRemindersApiList: public TRemindersApiFixture,
                         public ::testing::WithParamInterface<TParametrizeAction>
{
};

TEST_F(TRemindersApiFixture, HasReminderId) {
    TRemindersState state;
    CreateValidReminderProto("my-lovely-existed-id", +1000).Swap(state.AddReminders());
    TRemindersApi api{Now, state};

    EXPECT_EQ(api.HasReminderId("my-lovely-non-existed-id"), false);
    EXPECT_EQ(api.HasReminderId("my-lovely-existed-id"), true);
}

TEST_F(TRemindersApiFixture, FindReminderById) {
    TRemindersState state;
    CreateValidReminderProto("my-lovely-existed-id", +1000).Swap(state.AddReminders());
    TRemindersApi api{Now, state};

    EXPECT_EQ(api.FindReminderById("my-lovely-non-existed-id"), nullptr);
    const auto* r = api.FindReminderById("my-lovely-existed-id");
    EXPECT_TRUE(!!r);
    EXPECT_EQ(r->GetId(), "my-lovely-existed-id");
}

TEST_P(TRemindersApiAppend, ApiAppend) {
    const auto& test = GetParam();

    TRemindersState state;
    for (const auto& r : test.StateReminders) {
        CreateValidReminderProto(r.Id, r.RelTime).Swap(state.AddReminders());
    }

    TRemindersApi api{Now, state};
    api.RemoveOld();
    if (test.Append.Defined()) {
        const auto newReminder = CreateValidReminderProto(test.Append->Id, test.Append->RelTime);
        const auto status = api.Append(newReminder);

        ASSERT_EQ(status.IsSuccess(), true);
        ASSERT_EQ(status.Success().Reminder.ShortUtf8DebugString(), newReminder.ShortUtf8DebugString());

        const auto& rs = status.Success().State.GetReminders();
        ASSERT_EQ(rs.size(), (int)test.Checks.size());
        for (size_t i = 0, j = test.Checks.size(); i < j; ++i) {
            EXPECT_EQ(rs[i].GetId(), test.Checks[i]);
        }
    }
}

TEST_P(TRemindersApiList, ApiList) {
    const auto& test = GetParam();

    TRemindersState state;
    for (const auto& r : test.StateReminders) {
        CreateValidReminderProto(r.Id, r.RelTime).Swap(state.AddReminders());
    }

    TRemindersApi api{Now, state};
    api.RemoveOld();
    const auto status = api.ProcessList(CreateDateBounds(test.DateBounds.Get()).Get());
    ASSERT_EQ(status.IsSuccess(), true);

    const auto& reminders = status.Success().State.GetReminders();
    ASSERT_EQ(reminders.size(), (int)test.Checks.size());
    for (auto i = reminders.size() - 1; i >= 0; --i) {
        EXPECT_EQ(reminders[i].GetId(), test.Checks[i]);
    }
}

INSTANTIATE_TEST_SUITE_P(
    RemindersApi,
    TRemindersApiAppend,
    ::testing::Values(
        TParametrizeAction{
            .Title = "EmptyAndAppend",
            .Append = TParametrizeAction::TReminder{ "normal", 100 },
            .Checks = { "normal" },
        },
        TParametrizeAction{
            .Title = "RottenStateAndNormalAppend",
            .StateReminders = { { "rotten1", -200 }, { "rotten2", -100 } },
            .Append = TParametrizeAction::TReminder{ "normal", 100 },
            .Checks = { "normal" },
        },
        TParametrizeAction{
            .Title = "RottenNormalStateAndAppend",
            .StateReminders = { { "rotten1", -100 }, { "normal1", 100 } },
            .Append = TParametrizeAction::TReminder{ "normal2", 200 },
            .Checks = { "normal1", "normal2" },
        },
        TParametrizeAction{
            .Title = "MiddleAppend",
            .StateReminders = { { "normal1", 100 }, { "normal3", 300 } },
            .Append = TParametrizeAction::TReminder{ "normal2", 200 },
            .Checks = { "normal1", "normal2", "normal3" },
        },
        TParametrizeAction{
            .Title = "PushBack",
            .StateReminders = { { "normal1", 100 }, { "normal2", 200 } },
            .Append = TParametrizeAction::TReminder{ "normal3", 300 },
            .Checks = { "normal1", "normal2", "normal3" },
        },
        TParametrizeAction{
            .Title = "PushBegin",
            .StateReminders = { { "normal2", 100 }, { "normal3", 200 } },
            .Append = TParametrizeAction::TReminder{ "normal1", 50 },
            .Checks = { "normal1", "normal2", "normal3" },
        }
    ),
    &TestNameGenerator<TRemindersApiAppend>
);

INSTANTIATE_TEST_SUITE_P(
    RemindersApi,
    TRemindersApiList,
    ::testing::Values(
        TParametrizeAction{
            .Title = "EmptyList"
        },
        TParametrizeAction{
            .Title = "RottenDissapeared",
            .StateReminders = { { "rotten1", -200 }, { "rotten2", -100 }, { "normal1", 100 }, { "normal2", 200 } },
            .Checks = { "normal1", "normal2" }
        },
        TParametrizeAction{
            .Title = "EmptyAfterRottenDisappeared",
            .StateReminders = { { "rotten1", -200 }, { "rotten2", -100 } },
            .Checks = {}
        },
        TParametrizeAction{
            .Title = "FilterAllExistsNoAdjust",
            .StateReminders = { { "normal1", 100 }, { "normal2", 200 } },
            .Checks = { "normal1", "normal2" },
            .DateBounds = TParametrizeAction::TFilter{ 100, 200, false },
        },
        TParametrizeAction{
            .Title = "FilterOutEndNoAdjust",
            .StateReminders = { { "normal1", 100 }, { "normal2", 200 } },
            .Checks = { "normal1" },
            .DateBounds = TParametrizeAction::TFilter{ 100, 199, false },
        },
        TParametrizeAction{
            .Title = "FilterOutBeginNoAdjust",
            .StateReminders = { { "normal1", 100 }, { "normal2", 200 } },
            .Checks = { "normal2" },
            .DateBounds = TParametrizeAction::TFilter{ 101, 200, false },
        },
        TParametrizeAction{
            .Title = "FilterWithAdjust",
            .StateReminders = { { "normal1", 100 }, { "normal2", 200 } },
            .Checks = { "normal1", "normal2" },
            .DateBounds = TParametrizeAction::TFilter{ 100, 50, true },
        },
        TParametrizeAction{
            .Title = "BadFilterWithoutAdjust",
            .StateReminders = { { "normal1", 100 }, { "normal2", 200 } },
            .Checks = { },
            .DateBounds = TParametrizeAction::TFilter{ 100, 50, false },
        },
        TParametrizeAction{
            .Title = "FilterOutBeginNoAdjustWithRotten",
            .StateReminders = { { "rotten1", -100 }, { "normal1", 100 }, { "normal2", 200 } },
            .Checks = { "normal2" },
            .DateBounds = TParametrizeAction::TFilter{ 101, 200, false },
        }
    ),
    &TestNameGenerator<TRemindersApiList>
);

} // ns

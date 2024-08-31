#include "ut.h"

#include <alice/library/scenarios/reminders/memento.h>

#include <alice/megamind/protos/scenarios/request.pb.h>
#include <alice/memento/proto/api.pb.h>
#include <google/protobuf/any.pb.h>

namespace {

using namespace NAlice::NRemindersApi;
using namespace NAlice::NRemindersApi::NTesting;

TEST(RemindersMemento, ParseEmptyCopyRef) {
    const NAlice::NScenarios::TMementoData md;
    const auto state = RemindersFromMemento(md);
    EXPECT_EQ(state.Defined(), false);
}

TEST(RemindersMemento, ParseEmptyMove) {
    const auto state = RemindersFromMemento(NAlice::NScenarios::TMementoData{});
    EXPECT_EQ(state.Defined(), false);
}

TEST(RemindersMemento, ParseFromBase64EmptyString) {
    const auto state = RemindersFromMemento("");
    EXPECT_EQ(state.Defined(), false);
}

TEST_F(TRemindersApiFixture, ParseValidMementoFromProto) {
    NAlice::NScenarios::TMementoData md;
    auto& reminders = *md.MutableUserConfigs()->MutableReminders();
    CreateValidReminderProto("my-lovely-reminder", 0).Swap(reminders.AddReminders());

    const auto state = RemindersFromMemento(md);
    EXPECT_EQ(state.Defined(), true);
    EXPECT_EQ(state->ShortUtf8DebugString(), reminders.ShortUtf8DebugString());
}

TEST_F(TRemindersApiFixture, BuilderSmoke) {
    TRemindersState state;
    CreateValidReminderProto("my-lovely-reminder", 0).Swap(state.AddReminders());

    TMementoReminderDirectiveBuilder builder;
    const auto& directive = builder.BuildSaveServerDirective(state);

    ASSERT_EQ(directive.HasMementoChangeUserObjectsDirective(), true);

    const auto& uc = directive.GetMementoChangeUserObjectsDirective().GetUserObjects().GetUserConfigs();
    ASSERT_EQ(uc.size(), 1);

    const auto& ucState = uc[0];
    EXPECT_EQ(ucState.GetKey(), ru::yandex::alice::memento::proto::EConfigKey::CK_REMINDERS);

    TRemindersState stateAfterPack;
    ucState.GetValue().UnpackTo(&stateAfterPack);
    EXPECT_EQ(state.ShortUtf8DebugString(), stateAfterPack.ShortUtf8DebugString());
}

} // ns

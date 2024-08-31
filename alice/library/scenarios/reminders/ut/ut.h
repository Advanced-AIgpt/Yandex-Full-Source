#pragma once

#include <alice/library/scenarios/reminders/api.h>

#include <library/cpp/testing/gtest/gtest.h>

#include <util/datetime/base.h>
#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/string/builder.h>

namespace NAlice::NRemindersApi::NTesting {

struct TParametrizeAction {
    struct TReminder {
        TString Id;
        i64 RelTime;
    };
    struct TFilter {
        i64 From;
        i64 Till;
        bool Adjust;
    };
    TString Title;
    TVector<TReminder> StateReminders;
    TMaybe<TReminder> Append;
    TVector<TString> Checks;
    TMaybe<TFilter> DateBounds = Nothing();
};

class TRemindersApiFixture: public ::testing::Test {
protected:
    TInstant Now = TInstant::Now();

    TReminderProto CreateValidReminderProto(const TString& id, i64 diffSeconds) const;
    TMaybe<TDateBounds> CreateDateBounds(const TParametrizeAction::TFilter* bounds) const;
};

template <typename T>
TString TestNameGenerator(const ::testing::TestParamInfo<typename T::ParamType>& test) {
    return TStringBuilder{} << test.index << test.param.Title;
}

} // NAlice::NRemindersApi::NTesting

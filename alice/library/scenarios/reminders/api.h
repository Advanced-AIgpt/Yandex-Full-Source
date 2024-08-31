#pragma once

#include "common.h"

#include "datetime.h"

#include <infra/libs/outcome/result.h>

#include <util/datetime/base.h>
#include <util/generic/hash_set.h>
#include <util/generic/maybe.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>

#include <cstddef>

namespace NAlice::NRemindersApi {

class TRemindersApi {
public:
    enum class EError : ui8 {
        ReminderInPast /* "reminder_in_the_past" */,
        ReminderNotFound /* "reminder_not_found" */,
    };

    static constexpr std::nullptr_t WithoutDateBounds = nullptr;

public:
    struct TAppendResult {
        const TRemindersState& State;
        const TReminderProto& Reminder;
    };
    using TAppendStatus = TExpected<TAppendResult, EError>;

    class TStateResult {
    private:
        TMaybe<TRemindersState> OwnState_;

    public:
        TStateResult(const TRemindersState& state)
            : State{state}
        {
        }
        TStateResult(TRemindersState&& state)
            : OwnState_{std::move(state)}
            , State{OwnState_.GetRef()}
        {
        }
        TStateResult(TStateResult&& rhs)
            : OwnState_{std::move(rhs.OwnState_)}
            , State{OwnState_.Defined() ? OwnState_.GetRef() : rhs.State}
        {
        }
        TStateResult(const TStateResult& rhs) = delete;
        TStateResult& operator=(const TStateResult& rhs) = delete;
        TStateResult& operator=(TStateResult&& rhs) = delete;

        const TRemindersState& State;
    };
    using TStateStatus = TExpected<TStateResult, EError>;

    struct TRemoveResult {
        const TRemindersState& State;
    };
    using TRemoveStatus = TExpected<TRemoveResult, EError>;

public:
    /** Constructs reminders API from reminders state protobuf.
     *  TRemindersState.Reminder is expected to be **sorted** by ShootAt time!
     *  @param[in] now is a current server time
     *  @param[in] remindersState protobuf with current state of reminders
     */
    TRemindersApi(TInstant now, TRemindersState state);

    /** Remove old reminders e.g. which time is less than `now` (passed via constructor).
     */
    void RemoveOld();

    /** Create new reminder and append it to `TReminderState` proto.
     */
    TAppendStatus Append(const TString& guid, const TString& text, TInstant shootAt, const TString& timeZone);

    /** Append new reminder to the `TRemindersState` with regards of ShootAt time.
     *  @param[in] newReminder is a protobuf for new reminder (better to use std::move()).
     *  @return status which is expcted of `EError` enum and TAppendResult which contains ref to actual `TRemindersState` and a ref to the newly created reminder proto.
     */
    TAppendStatus Append(TReminderProto newReminder);

    /** Right now it's just returns result with current `TRemindersState` proto with filtered out reminders.
     * @param[in] dateBounds if specified used to return reminders within these datetime bounds.
     */
    TStateStatus ProcessList(const TDateBounds* dateBounds) const;

    TRemoveStatus Clear();
    TRemoveStatus Remove(const THashSet<TStringBuf>& reminderId);

    /** Check if reminder id is in reminders' state.
     * @param[in] id is a checking reminder id.
     * return true if exists.
     */
    bool HasReminderId(TStringBuf id) const;

    const TReminderProto* FindReminderById(TStringBuf id) const;

private:
    TInstant Now_;
    TRemindersState State_;
};

} // namespace NAlice::NRemindersApi

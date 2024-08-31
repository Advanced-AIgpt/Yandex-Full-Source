#include "api.h"

// Protobufs.
#include <alice/megamind/protos/common/frame.pb.h>
#include <alice/megamind/protos/scenarios/request.pb.h>

#include <util/generic/algorithm.h>
#include <util/generic/hash_set.h>
#include <util/string/cast.h>

namespace NAlice::NRemindersApi {

TRemindersApi::TRemindersApi(TInstant now, TRemindersState state)
    : Now_{now}
    , State_{std::move(state)}
{
}

void TRemindersApi::RemoveOld() {
    const auto nowEpoch = Now_.Seconds();
    auto& rs = *State_.MutableReminders();

    // We assume that `rs` is sorted by GetShootAt().
    // Search for the first reminder in future and remove everyting before.
    const auto isInFuture = [nowEpoch](const auto& r) {
        return r.GetShootAt() > nowEpoch;
    };
    if (auto it = FindIf(rs.begin(), rs.end(), isInFuture); it != rs.end()) {
        rs.erase(rs.begin(), it);
    } else {
        rs.Clear();
    }
}


TRemindersApi::TAppendStatus TRemindersApi::Append(const TString& guid, const TString& text,
                                                   TInstant shootAt, const TString& timeZone)
{
    TReminderProto reminderProto;
    reminderProto.SetId(guid);
    reminderProto.SetText(text);
    reminderProto.SetShootAt(shootAt.Seconds());
    reminderProto.SetTimeZone(timeZone);
    return Append(std::move(reminderProto));
}

// TODO (petrk) add checking for fields not empty and not in the past.
TRemindersApi::TAppendStatus TRemindersApi::Append(TReminderProto newReminder) {
    if (TInstant::Seconds(newReminder.GetShootAt()) < Now_) {
        return EError::ReminderInPast;
    }

    auto& rs = *State_.MutableReminders();
    newReminder.Swap(rs.Add());

    size_t i = rs.size() - 1;
    for (/* nop */; i > 0 && rs[i].GetShootAt() < rs[i-1].GetShootAt(); --i) {
        rs.SwapElements(i, i-1);
    }

    return TAppendResult{State_, State_.GetReminders()[i]};
}

TRemindersApi::TStateStatus TRemindersApi::ProcessList(const TDateBounds* bounds) const {
    if (!bounds) {
        return TStateResult{State_};
    }

    TRemindersState state;
    auto& reminders = *state.MutableReminders();
    for (const auto& r : State_.GetReminders()) {
        if (bounds->IsIn(TInstant::Seconds(r.GetShootAt()))) {
            reminders.Add()->CopyFrom(r);
        }
    }

    return TStateResult{std::move(state)};
}

TRemindersApi::TRemoveStatus TRemindersApi::Clear() {
    TRemoveResult result{State_};
    State_.ClearReminders();
    return result;
}

TRemindersApi::TRemoveStatus TRemindersApi::Remove(const THashSet<TStringBuf>& ids) {
    auto& rs = *State_.MutableReminders();

    const auto isForDeletion = [&ids](const auto& elem) {
        return ids.count(elem.GetId());
    };
    rs.erase(std::remove_if(rs.begin(), rs.end(), isForDeletion), rs.end());

    return TRemoveResult{State_};
}

bool TRemindersApi::HasReminderId(TStringBuf id) const {
    return FindReminderById(id) != nullptr;
}

const TReminderProto* TRemindersApi::FindReminderById(TStringBuf id) const {
    const auto& rs = State_.GetReminders();
    return FindIfPtr(rs.begin(), rs.end(), [id](const auto& r) { return r.GetId() == id; });
}

} // namespace NAlice::NRemindersApi

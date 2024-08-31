#pragma once

#include "constants.h"
#include "request.h"

#include <alice/bass/forms/context/fwd.h>

#include <alice/library/scenarios/reminders/datetime.h>

#include <library/cpp/timezone_conversion/civil.h>

#include <util/generic/strbuf.h>
#include <util/generic/maybe.h>

namespace NBASS::NReminders {

inline constexpr TStringBuf FAIL_PAST_DATETIME = "past_datetime";

class TRemindersListScroll {
public:
    i64 Offset;
    i64 PerPage;
    TMaybe<NSc::TValue> IterationKey;

public:
    // not by const because we remove scroll json from slot value
    explicit TRemindersListScroll(TAnswerSlot* slot);

    void UpdateFromApi(const NSc::TValue& apiJson);

    void WriteToContext(TRequest* req) const;

    bool HasMore() const {
        return !IterationKey.Defined() || !IterationKey->IsNull();
    }
};

void ReminderCreate(TContext& ctx);
void ReminderCancel(TContext& ctx);
void RemindersListCancel(TContext& ctx);
void RemindersListCancelStop(TContext& ctx);
void ReminderLanding(TContext& ctx);

bool ReminderCleanupForTestUser(TContext& ctx);

// Reminder's helpers.
void TakeDayPartIntoAccount(TContext& ctx);

TMaybe<NDatetime::TCivilSecond> ProcessShootTimeValue(TContext& ctx, TRequest& req);

class IRemindersBackend {
public:
    using TDateBounds = NAlice::NRemindersApi::TDateBounds;

    class TRemindersListWrapper {
    public:
        TRemindersListWrapper(const NSc::TValue& reminders, bool isEverything)
            : Reminders_{reminders}
            , IsEverything_{isEverything}
        {
        }

        bool IsEverything() const {
            return IsEverything_;
        }

        template <typename T>
        bool ForEach(T&& func) const {
            for (const auto& reminder : Reminders_.GetArray()) {
                if (!func(reminder["id"].GetString())) {
                    return false;
                }
            }
            return true;
        }

    private:
        const NSc::TValue& Reminders_;
        bool IsEverything_;
    };

public:
    IRemindersBackend(TRequest& req, TContext& ctx)
        : Req_{req}
        , Ctx_{ctx}
    {
    }
    virtual ~IRemindersBackend() = default;

    void ProcessList();
    void ProcessCancel();

protected:
    // Helpers.
    bool TryHandleCancellation();
    bool IsCancellationRequired() const;
    bool TryFillDateIfOnlyTimeIsProvided();

    struct TRemindersResult {
        TRemindersResult() = default;
        TRemindersResult(NSc::TValue&& list, i64 total)
            : List{std::move(list)}
            , Total{total}
        {
        }

        NSc::TValue List;
        i64 Total = 0;
    };

    TMaybe<TRemindersResult> GetReminders(bool soonestFirst, TRemindersListScroll* scroll);

protected:
    virtual bool IsCancellationSupported() const = 0;
    virtual void SuggestForListing() = 0;
    virtual bool TryCancel(const TRemindersListWrapper& listWrapper) = 0;


    virtual TMaybe<TRemindersResult> GetRemindersList(bool soonestFirst,
                                                      const TDateBounds* dateBounds,
                                                      TRemindersListScroll* scroll) = 0;

    virtual void PostProcessList(const TRemindersResult& /* reminders */) {
    }

protected:
    TRequest& Req_;
    TContext& Ctx_;
};


} // namespace NBASS::NReminders

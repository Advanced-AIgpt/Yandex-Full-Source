/** TODO fixlist
 * неполные даты не работают: поставить напоминание на сентябрь, купить что то
 * ответ как утро или вечер
 *
 * завести еще один тип ошибки, что время не в прошлом, а не подходящее
 */
#include "reminder.h"

#include "device_reminders.h"
#include "helpers.h"
#include "memento_reminders.h"
#include "request.h"

#include <alice/library/scenarios/alarm/date_time.h>
#include <alice/library/scenarios/alarm/helpers.h>

#include <alice/bass/forms/common/directives.h>

#include <alice/bass/forms/context/context.h>
#include <alice/bass/forms/directives.h>

#include <alice/bass/libs/logging_v2/logger.h>

#include <alice/library/analytics/common/product_scenarios.h>

#include <library/cpp/timezone_conversion/civil.h>

#include <util/datetime/parser.h>
#include <util/generic/guid.h>
#include <util/generic/hash.h>
#include <util/generic/scope.h>
#include <util/random/fast.h>
#include <util/system/types.h>

#include <functional>

namespace NBASS {
namespace NReminders {

namespace {

namespace NAlarm = NAlice::NScenarios::NAlarm;

constexpr TStringBuf FAIL_INCOMPLETE_DATE = "incomplete_datetime";
constexpr TStringBuf FAIL_INVALID_DATETIME = "invalid_datetime";
constexpr TStringBuf FAIL_NO_TIME = "no_time";

constexpr TStringBuf SLOT_NAME_ACTION = "action";
constexpr TStringBuf SLOT_NAME_IS_FORBIDDEN = "is_forbidden";
constexpr TStringBuf SLOT_NAME_OFFSET = "offset";
constexpr TStringBuf SLOT_NAME_REMINDER_ID = "push_reminder_id";
constexpr TStringBuf SLOT_NAME_REMINDER_ID_TO_CANCEL = "reminder_id";
constexpr TStringBuf SLOT_NAME_REMINDER_ID_TO_CREATE_FOR_TESTING = "id_to_create_for_testing";
constexpr TStringBuf SLOT_TYPE_BOOL = "bool";
constexpr TStringBuf SLOT_TYPE_CANCEL_ACTION = "cancel_action";
constexpr TStringBuf SLOT_TYPE_CREATE = "create_reminder_ans";
constexpr TStringBuf SLOT_TYPE_LIST = "list_reminder_ans";
constexpr TStringBuf SLOT_TYPE_NUM = "num";
constexpr TStringBuf SLOT_TYPE_REMINDER_ID = "string";
constexpr TStringBuf SLOT_TYPE_SELECTION = "selection";
constexpr TStringBuf SLOT_TYPE_STRING = "string";

constexpr TStringBuf PUSH_TITLE = "Алиса: напоминание";
constexpr TStringBuf USER_AGENT = "YaBass-1.0";

constexpr i64 ITEMS_PER_PAGE_FIRST = 3;
constexpr i64 ITEMS_PER_PAGE_AFTER_FIRST = 3;
constexpr i64 ITEMS_PER_PAGE_MIN_MAX = Min(ITEMS_PER_PAGE_FIRST, ITEMS_PER_PAGE_AFTER_FIRST);
const NDatetime::TDiff PAST_BAD_DURATIONS_DAYS = 30;

bool RemindersV1CheckIfEnabled(TContext& ctx);
void RemindersV1CreateHandler(TRequest& req, TContext& ctx);
void RemindersV1ListHandler(TRequest& req, TContext& ctx);
void RemindersV1CancelHandler(TRequest& req, TContext& ctx);

const TVector<TRequest::THandler> CANCEL_HANDLERS = {
    {
        MementoRemindersCheckIfEnabled,
        MementoRemindersCancelHandler,
        {
            TError::EType::REMINDERERROR,
            SLOT_TYPE_CREATE,
            "reminders",
            "cancel",
            "canceling_failed",
        },
    },
    {
        DeviceLocalRemindersCheckIfEnabled,
        DeviceLocalRemindersCancelHandler,
        {
            TError::EType::REMINDERERROR,
            SLOT_TYPE_CREATE,
            "reminders",
            "cancel",
            "canceling_failed",
        },
    },
    {
        RemindersV1CheckIfEnabled,
        RemindersV1CancelHandler,
        {
            TError::EType::REMINDERERROR,
            SLOT_TYPE_CREATE,
            "reminders",
            "cancel",
            "canceling_failed",
        }
    },
};

const TVector<TRequest::THandler> CREATE_HANDLERS = {
    {
        MementoRemindersCheckIfEnabled,
        MementoRemindersCreateHandler,
        {
            TError::EType::REMINDERERROR,
            SLOT_TYPE_CREATE,
            "reminders",
            "processing",
            "creating_failed",
            Nothing(),
            false,
            true, // IsUidRequired
        }
    },
    {
        DeviceLocalRemindersCheckIfEnabled,
        DeviceLocalRemindersCreateHandler,
        {
            TError::EType::REMINDERERROR,
            SLOT_TYPE_CREATE,
            "reminders",
            "processing",
            "creating_failed",
            Nothing(),
            false,
            false, // IsUidRequired
        }
    },
    {
        RemindersV1CheckIfEnabled,
        RemindersV1CreateHandler,
        {
            TError::EType::REMINDERERROR,
            SLOT_TYPE_CREATE,
            "reminders",
            "processing",
            "creating_failed",
        }
    }
};

const TVector<TRequest::THandler> LIST_HANDLERS = {
    {
        MementoRemindersCheckIfEnabled,
        MementoRemindersListHandler,
        {
            TError::EType::REMINDERERROR,
            SLOT_TYPE_LIST,
            "reminders",
            "voice",
            "list_failed",
            Nothing(),
            false,
            true /* login */
        }
    },
    {
        DeviceLocalRemindersCheckIfEnabled,
        DeviceLocalRemindersListHandler,
        {
            TError::EType::REMINDERERROR,
            SLOT_TYPE_LIST,
            "reminders",
            "voice",
            "list_failed",
            Nothing(),
            false,
            false
        }
    },
    {
        [](TContext& ctx) -> bool {
            return RemindersV1CheckIfEnabled(ctx) && TRequest::IsSearchAppBehavior(ctx.MetaClientInfo());
        },
        RemindersV1ListHandler,
        {
            TError::EType::REMINDERERROR,
            SLOT_TYPE_LIST,
            "reminders",
            "textandvoice",
            "list_failed",
        }
    },
    {
        [](auto& ctx) { return TRequest::IsSearchAppBehavior(ctx.MetaClientInfo()); },
        RemindersV1ListHandler,
        {
            TError::EType::REMINDERERROR,
            SLOT_TYPE_LIST,
            "reminders",
            "textandvoice",
            "list_failed",
        }
    },
    {
        [](auto& /* ctx */) { return true; },
        RemindersV1ListHandler,
        {
            TError::EType::REMINDERERROR,
            SLOT_TYPE_LIST,
            "reminders",
            "voice",
            "list_failed",
        }
    }
};

const TFormsDescr DESCR_LIST_TEXT_AND_VOICE {
    TError::EType::REMINDERERROR,
    SLOT_TYPE_LIST,
    "reminders",
    "textandvoice",
    "list_failed",
};

const TFormsDescr DESCR_LIST_VOICE {
    TError::EType::REMINDERERROR,
    SLOT_TYPE_LIST,
    "reminders",
    "voice",
    "list_failed",
};

const TFormsDescr DESCR_PUSH_LANDING {
    TError::EType::REMINDERERROR,
    "alarm_reminder_ans",
    "reminders",
    "push",
    "alarm_reminders_failed",
};

const TFormsDescr DESCR_REMOVE_ALL {
    TError::EType::REMINDERERROR,
    "alarm_reminder_ans",
    "reminders",
    "remove_all",
    "alarm_reminders_failed",
    Nothing(),
    true /* do not check for supported */
};

enum class EReminderFailState {
    Incomplete,
    InPast,
    Invalid,
};

template <typename R>
R GetReminderDate(const NDatetime::TCivilSecond& now, const NAlarm::TDate& date,
                  const TMaybe<NAlarm::TDayTime>& time,
                  std::function<R(NDatetime::TCivilSecond)> onSuccess,
                  std::function<R(EReminderFailState)> onFail)
{
    if (!date.HasExactDay()) {
        return onFail(EReminderFailState::Incomplete);
    }

    NDatetime::TCivilSecond trigger{date.Apply(now)};
    if (date.HasOnlyWeekday() && NAlarm::SameDate(now, trigger) && time && time->Apply(now) < now) {
        trigger = NDatetime::AddDays(trigger, 7);
    }

    if (trigger >= now) {
        return onSuccess(trigger);
    }

    // If alarm is in the past and days are relative, we are sure that user definitely requested date in the past.
    if (date.Days && date.Days->Relative) {
        return onFail(EReminderFailState::InPast);
    }

    if (!date.Years) {
        if (date.Months && date.Months->Relative && date.Months->Value < 0) {
            return onFail(EReminderFailState::InPast);
        }

        const NDatetime::TCivilDiff diff = NDatetime::GetCivilDiff(now, trigger, NDatetime::ECivilUnit::Day);
        if (diff.Value > 0 && diff.Value < PAST_BAD_DURATIONS_DAYS) {
            return onFail(EReminderFailState::InPast);
        }

        trigger = NDatetime::AddYears(trigger, 1);
    }

    return onSuccess(trigger);
}

TMaybe<NSc::TValue> RequestApi(TRequest& req, std::function<void(NHttpFetcher::TRequest&)> updateRequest) {
    NHttpFetcher::TRequestPtr r = req.Ctx.GetSources().RemindersApi(
        TStringBuilder() << TStringBuf("/api/v1/") << req.Uid << TStringBuf("/reminders/yandex-calendar.json")
    ).Request();
    if (!r) {
        LOG(ERR) << "Fetch reminders api: request object is null" << Endl;
        req.Descr().SetDefaultError(req.Ctx);
        return Nothing();
    }

    r->AddHeader("user-agent", USER_AGENT).SetContentType("application/json");
    updateRequest(*r);

    NHttpFetcher::THandle::TRef h = r->Fetch();
    if (!h) {
        LOG(ERR) << "Fetch reminders api: handle object is null" << Endl;
        req.Descr().SetDefaultError(req.Ctx);
        return Nothing();
    }

    NHttpFetcher::TResponse::TRef resp = h->Wait();
    if (!resp) {
        LOG(ERR) << "Fetch reminders api: response object is null" << Endl;
        req.Descr().SetDefaultError(req.Ctx);
        return Nothing();
    }

    if (resp->IsError()) {
        LOG(ERR) << "Fetch reminders api: " << resp->GetErrorText() << ": " << resp->Data << Endl;
        req.Descr().SetDefaultError(req.Ctx);
        return Nothing();
    }

    try {
        return NSc::TValue::FromJsonThrow(resp->Data);
    } catch (const NSc::TSchemeParseException& e) {
        LOG(ERR) << "ParseJsonRemindersApi: " << e.Offset << ", " << e.Reason << ": " << resp->Data << Endl;
        req.Descr().SetDefaultError(req.Ctx);
        return Nothing();
    }

    return Nothing();
}

void AddCancelSuggest(TRequest& req, TStringBuf id) {
    TContext formUpdate(req.Ctx, REMINDERS_FORM_NAME_CANCEL);
    NSc::TValue& json = CANCEL_HANDLERS[1].Descr.CreateSlotInContext(formUpdate);
    json["id"].SetString(id);
    json["type"].SetString(CANCEL_HANDLERS[1].Descr.DefaultType);
    req.Ctx.AddSuggest(TStringBuf("reminders__cancel"),
                       NSc::Null(),
                       formUpdate.ToJson(
                           TContext::EJsonOut::TopLevel
                           | TContext::EJsonOut::Resubmit
                       )
    );
}

TString CreateUserListUrl(TStringBuf tz) {
    static constexpr TStringBuf host = "https://calendar.yandex.ru/todo";
    TCgiParameters cgi;
    cgi.InsertUnescaped("client", "alice");
    cgi.InsertUnescaped("tab", "reminders");
    cgi.InsertUnescaped("tz", tz);
    return TStringBuilder() << host << '?' << cgi.Print();
}

void AddUrlSuggest(TContext& ctx, TStringBuf type, TStringBuf url) {
    NSc::TValue dataSuggest;
    dataSuggest["url"].SetString(url);
    ctx.AddSuggest(type, std::move(dataSuggest));
}

TMaybe<IRemindersBackend::TDateBounds> GetDateFilter(const TRequest& req) {
    using namespace NDatetime;

    const TSlot* const dateSlot = req.Ctx.GetSlot(SLOT_NAME_DATE, SLOT_TYPE_DATE);
    const TSlot* const timeSlot = req.Ctx.GetSlot(SLOT_NAME_TIME, SLOT_TYPE_TIME);

    const bool dateSlotEmpty = IsSlotEmpty(dateSlot);
    const bool timeSlotEmpty = IsSlotEmpty(timeSlot);
    if (dateSlotEmpty && timeSlotEmpty) {
        return Nothing();
    }

    const TCivilSecond curDateTime = req.CurrentTime();

    TMaybe<NAlarm::TDate> dateFromSlot;
    if (!dateSlotEmpty) {
        dateFromSlot = NAlarm::TDate::FromValue(dateSlot->Value);
    }

    if (!dateFromSlot) {
        dateFromSlot = NAlarm::TDate::MakeToday();
    }

    const TCivilDay day{dateFromSlot->Apply(curDateTime)};

    TMaybe<NAlarm::TDayTime> dayTime;
    if (!IsSlotEmpty(timeSlot)) {
        dayTime = NAlarm::TDayTime::FromValue(timeSlot->Value);
    }

    TMaybe<IRemindersBackend::TDateBounds> bounds;

    if (dayTime) {
        bounds.ConstructInPlace(curDateTime, dayTime->Apply(curDateTime), req.UserTZ);
    } else if (day == TCivilDay{curDateTime}) {
        bounds.ConstructInPlace(curDateTime, AddSeconds(AddDays(TCivilDay{curDateTime}, 1), -1), req.UserTZ);
    } else {
        bounds.ConstructInPlace(day, AddSeconds(AddDays(TCivilDay{day}, 1), -1), req.UserTZ);
    }

    bounds->AdjustRightBound();

    return bounds;
}

void AddSuggestForGrantingPushPermission(TContext& ctx) {
    NSc::TValue data;
    NSc::TValue& command = data["commands"].SetArray().Push();
    command["command_type"].SetString("request_permissions");
    NSc::TValue& payload = command["data"];
    payload["permissions"].SetArray().Push("push_notifications");
    payload["on_success"].SetArray();
    payload["on_fail"].SetArray();
    ctx.AddSuggest(TStringBuf("reminders__grant_permission"), std::move(data));
}

TString GenerateNewReminderId(const TContext& ctx) {
    const TSlot* const idSlot = ctx.GetSlot(SLOT_NAME_REMINDER_ID_TO_CREATE_FOR_TESTING, SLOT_TYPE_STRING);
    return ctx.IsTestUser() && !IsSlotEmpty(idSlot) ? idSlot->Value.GetString().data() : CreateGuidAsString();
}

bool RemindersV1CheckIfEnabled(TContext& ctx) {
    if (ctx.ClientFeatures().SupportsSynchronizedPush()) {
        return true;
    }

    ctx.CreateSlot(SLOT_NAME_IS_FORBIDDEN, SLOT_TYPE_BOOL, true /* optional */, true /* value */);
    ctx.AddOnboardingSuggest();
    return false;
}

void SetReminderProductScenario(TContext& ctx) {
    ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::REMINDER);
}

bool ShouldAskToCheckPushPermission(const TContext& ctx) {
    return ctx.MetaClientInfo().IsIOS() &&
        !ctx.HasExpFlag(EXPERIMENTAL_FLAG_REMINDERS_NO_ASK_CHECK_PUSH_PERMISSION) &&
        ctx.GetPermissionInfo(EClientPermission::PushNotifications) != EPermissionType::Permitted;
}

bool ShouldRequestPushPermission(const TContext& ctx) {
    return ctx.HasExpFlag(EXPERIMENTAL_FLAG_REMINDERS_CHECK_PUSH_PERMISSION) && ctx.MetaClientInfo().IsIOS() &&
        ctx.GetPermissionInfo(EClientPermission::PushNotifications) == EPermissionType::Forbidden;
}


void RemindersV1CreateHandler(TRequest& req, TContext& ctx) {
    using namespace NAlice::NScenarios::NAlarm;
    if (ShouldRequestPushPermission(ctx)) {
        AddSuggestForGrantingPushPermission(ctx);
        ctx.AddAttention("request_push_permission");
        ctx.AddStopListeningBlock();
        return;
    }

    TakeDayPartIntoAccount(ctx);

    req.MoveSlotValueToAnswer(SLOT_NAME_WHAT, SLOT_TYPE_WHAT);
    NSc::TValue& whatValue = req.AnswerSlot().Json()[SLOT_NAME_WHAT];
    if (whatValue.GetString().empty()) {
        if (const auto value = req.ProcessEmptySlotWhatValue(); value.Defined()) {
            whatValue.SetString(*value);
        } else {
            return;
        }
    }

    TMaybe<NDatetime::TCivilSecond> shootTime = ProcessShootTimeValue(ctx, req);
    if (!shootTime.Defined()) {
        return;
    }

    LOG(DEBUG) << "Reminder date is '" << *shootTime << Endl;

    const TString id = GenerateNewReminderId(ctx);

    NSc::TValue apiJson;
    apiJson["name"].SetString(whatValue.GetString());
    apiJson["reminderDate"].SetString(NDatetime::Format("%Y-%m-%dT%H:%M:%S%Ez", *shootTime, req.UserTZ));

    NSc::TValue& supChannel = apiJson["channels"]["sup"];
    TContext greetingCtx{ctx, REMINDERS_FORM_NAME_PUSH_LANDING};
    greetingCtx.CreateSlot(SLOT_NAME_REMINDER_ID, SLOT_TYPE_REMINDER_ID, true, NSc::TValue(id));
    NSc::TValue directives;
    directives.Push(greetingCtx.ToJson(TContext::EJsonOut::Resubmit | TContext::EJsonOut::FormUpdate | TContext::EJsonOut::ServerAction));
    TCgiParameters cgi;
    cgi.InsertUnescaped("directives", directives.ToJson());
    supChannel["uri"].SetString(TStringBuilder() << "dialog://?" << cgi.Print());
    supChannel["title"].SetString(PUSH_TITLE);
    supChannel["session_type"].SetString(TStringBuf("voice"));
    if (ctx.Meta().HasDeviceId() && !ctx.MetaClientInfo().IsSmartSpeaker()) {
        supChannel["device_id"].SetString(ctx.Meta().DeviceId());
    }
    if (ctx.Meta().HasUUID()) {
        supChannel["uuid"].SetString(ctx.Meta().UUID());
    }
    if (!ctx.MetaClientInfo().Name.empty()) {
        supChannel["app_id"].SetString(ctx.MetaClientInfo().Name);
    }
    if (!ctx.MetaClientInfo().Manufacturer.empty()) {
        supChannel["device_manufacturer"].SetString(ctx.MetaClientInfo().Manufacturer);
    }

    const TMaybe<NSc::TValue> respJson = RequestApi(
        req,
        [&apiJson, &id](NHttpFetcher::TRequest& r) {
            r.AddCgiParam("id", id);
            r.SetBody(apiJson.ToJson(), "PUT");
        }
    );
    if (!respJson) {
        return;
    }

    const TString createdId{(*respJson)["result"]["id"].ForceString()};
    if (!createdId) {
        LOG(ERR) << "Unable to add reminder (id not found in reminder api response): " << *respJson << Endl;
        return req.SetDefaultError();
    }

    req.AnswerSlot().Json()["id"].SetString(createdId);

    req.MoveSlotValueToAnswer(SLOT_NAME_TIME, SLOT_TYPE_TIME);
    req.MoveSlotValueToAnswer(SLOT_NAME_DATE, SLOT_TYPE_DATE);
    req.AnswerSlot().Json()[SLOT_NAME_DATE] = DateToValue(req.CurrentTime(), *shootTime);
    req.AnswerSlot().Json()[SLOT_NAME_TIME] = TimeToValue(*shootTime);

    req.CopySlotValueFromAnswer(SLOT_NAME_WHAT, SLOT_TYPE_WHAT);
    req.CopySlotValueFromAnswer(SLOT_NAME_DATE, SLOT_TYPE_DATE);
    req.CopySlotValueFromAnswer(SLOT_NAME_TIME, SLOT_TYPE_TIME);

    req.AnswerSlot().Json()["type"].SetString("ok");

    AddCancelSuggest(req, createdId);
    AddUrlSuggest(ctx, "reminders__show_reminders_list", CreateUserListUrl(ctx.Meta().TimeZone()));

    if (ShouldAskToCheckPushPermission(ctx)) {
        ctx.AddTextCardBlock("ask_to_turn_on_pushes");
    }

    ctx.AddStopListeningBlock();
}

void DeleteAllCancelationConfirmationFlags(TRequest& request) {
    auto& answerJson = request.AnswerSlot().Json();
    answerJson.Delete("all_cancelation_confirmation");
    answerJson.Delete("all_local_cancelation_confirmation");
}

void DeleteTemporalFlags(TRequest& request) {
    auto& answerJson = request.AnswerSlot().Json();
    answerJson.Delete("invalid_id");
    answerJson.Delete("id_without_cancelation");
}

NSc::TValue NormalizeTimeValue(TRequest& request, const NSc::TValue& time) {
    TMaybe<NAlarm::TDayTime> dayTime = NAlice::NScenarios::NAlarm::TDayTime::FromValue(time);
    const NDatetime::TCivilSecond curDateTime = request.CurrentTime();
    return dayTime ? NAlarm::TimeToValue(dayTime->Apply(curDateTime)) : NSc::Null();
}

void SetDefaultAction(TContext& ctx) {
    ctx.GetOrCreateSlot(SLOT_NAME_ACTION, SLOT_TYPE_CANCEL_ACTION)->Value.SetNull();
}

void RecordSuccessfulCancelation(TAnswerSlot& slot) {
    slot.Json()["canceled"].SetString("yes");
}

bool IsIdAllSpecified(const TContext& ctx) {
    const TSlot* selectionIdSlot = ctx.GetSlot(SLOT_NAME_REMINDER_ID_TO_CANCEL, SLOT_TYPE_SELECTION);
    return !IsSlotEmpty(selectionIdSlot) && selectionIdSlot->Value.GetString() == "all";
}

void ConfirmAllCancelation(TRequest& request) {
    request.AnswerSlot().Json()["all_cancelation_confirmation"].SetString("yes");
}

void ConfirmAllLocalCancelation(TRequest& request) {
    request.AnswerSlot().Json()["all_local_cancelation_confirmation"].SetString("yes");
}

bool IsAllCancelationConfirmed(TRequest& request) {
    return request.AnswerSlot().Json().TrySelect("all_cancelation_confirmation").GetString() == "yes";
}

bool IsAllLocalCancelationConfirmed(TRequest& request) {
    return request.AnswerSlot().Json().TrySelect("all_local_cancelation_confirmation").GetString() == "yes";
}

bool IsOneItemLeft(TRequest& request) {
    const TSlot* const offsetSlot = request->GetSlot(SLOT_NAME_OFFSET, SLOT_TYPE_NUM);
    const NSc::TValue& totalCount = request.AnswerSlot().Json().TrySelect("total_reminders_count");
    if (IsSlotEmpty(offsetSlot) || totalCount.IsNull()) {
        return false;
    }
    return totalCount.GetIntNumber() - offsetSlot->Value.GetIntNumber() == 1;
}

TMaybe<i64> GetNumNotShownRemindersYet(TRequest& request) {
    const TSlot* const offsetSlot = request->GetSlot(SLOT_NAME_OFFSET, SLOT_TYPE_NUM);
    const NSc::TValue& totalCount = request.AnswerSlot().Json().TrySelect("total_reminders_count");
    const NSc::TValue& reminders = request.AnswerSlot().Json().TrySelect("reminders");
    if (IsSlotEmpty(offsetSlot) || totalCount.IsNull() || reminders.IsNull()) {
        return Nothing();
    }
    return totalCount.GetIntNumber() - (offsetSlot->Value.GetIntNumber() + reminders.ArraySize());
}

bool IsFirstRequestWithinListOrCancelScenario(const TContext& ctx) {
    return ctx.FormName() == REMINDERS_FORM_NAME_LIST_CANCEL;
}

bool WasAnswerSlotReset(const TContext& ctx) {
    return ctx.FormName() == REMINDERS_FORM_NAME_LIST_CANCEL_RESETTING;
}

class TRemindersBackendV1 : public IRemindersBackend {
public:
    using IRemindersBackend::IRemindersBackend;

protected:
    TMaybe<TRemindersResult> GetRemindersList(bool soonestFirst,
                                              const TDateBounds* dateBounds,
                                              TRemindersListScroll* scroll) override
    {
        auto result = RequestApi(
            Req_,
            [soonestFirst, dateBounds, scroll](NHttpFetcher::TRequest& r) {
                if (scroll) {
                    r.AddCgiParam("count", ToString(scroll->PerPage));

                    if (scroll->IterationKey && !scroll->IterationKey->IsNull()) {
                        r.AddCgiParam("iteration-key", scroll->IterationKey->GetString());
                    }
                }
                if (soonestFirst) {
                    r.AddCgiParam("soonest_first", "true");
                }
                if (dateBounds) {
                    constexpr TStringBuf exactTimeFormat = "%Y-%m-%dT%H:%M:%S%Ez";

                    const auto df = TString::Join(dateBounds->FormatFrom(exactTimeFormat),
                                                  '/',
                                                  dateBounds->FormatTill(exactTimeFormat));
                    r.AddCgiParam("interval", df);
                }
            }
        );

        if (!result) {
            return Nothing();
        }

        const NSc::TValue& errorJson = (*result)["error"];
        if (!errorJson.IsNull()) {
            LOG(ERR) << "Error in response of reminders api: " << errorJson << Endl;
            Req_.SetDefaultError();
            return Nothing();
        }

        const i64 total = (*result)["result"]["totalCount"].GetIntNumber(-1);
        if (total < 0) {
            LOG(ERR) << "Error in response of reminders api: " << errorJson << Endl;
            Req_.SetDefaultError();
            return Nothing();
        }

        if (scroll) {
            scroll->UpdateFromApi(*result);
        }

        return TRemindersResult{std::move((*result)["result"]["reminders"]), total};
    }

    bool IsCancellationSupported() const override {
        return Ctx_.MetaClientInfo().IsSmartSpeaker();
    }

    void SuggestForListing() override {
        if (TRequest::IsSearchAppBehavior(Ctx_.MetaClientInfo())) {
            const TString webViewUrl{CreateUserListUrl(Ctx_.Meta().TimeZone())};
            AddUrlSuggest(Ctx_, "reminders__open_uri", webViewUrl);

            NSc::TValue payload;
            payload["uri"].SetString(webViewUrl);
            Ctx_.AddCommand<TReminderShowDirective>(TStringBuf("open_uri"), std::move(payload));
        }

        auto& answerJson = Req_.AnswerSlot().Json();
        if (!answerJson.Has("canceled") && !IsAllCancelationConfirmed(Req_)) {
            TMaybe<i64> numNotShownYet = GetNumNotShownRemindersYet(Req_);
            if (IsCancellationRequired() && numNotShownYet && *numNotShownYet > 0) {
                NSc::TValue data;
                data["num_reminders_to_show_next"].SetIntNumber(Min(ITEMS_PER_PAGE_AFTER_FIRST, *numNotShownYet));
                Ctx_.AddSuggest("reminders__scroll_next", std::move(data));
            }

            const NSc::TValue& reminders = Req_.AnswerSlot().Json().TrySelect("reminders");
            if (!reminders.IsNull() && reminders.ArraySize() > 1) {
                ui32 randInt = TFastRng<ui32>(Ctx_.Meta().HasEpoch() ? Ctx_.Meta().Epoch() : 0).GenRand();
                i64 randId = randInt % reminders.ArraySize() + 1;
                NSc::TValue data;
                data["id"].SetIntNumber(randId);
                Ctx_.AddSuggest("reminders__cancel_by_id", std::move(data));

                if (!IsAllLocalCancelationConfirmed(Req_)) {
                    Ctx_.AddSuggest("reminders__cancel_all");
                }
            }

            if (!IsCancellationRequired() && IsOneItemLeft(Req_)) {
                Ctx_.AddSuggest("reminders__cancel_single");
            }
        }

        const NSc::TValue& totalCount = Req_.AnswerSlot().Json().TrySelect("total_reminders_count");
        if (!IsCancellationRequired() || answerJson.Has("canceled") ||
            (!totalCount.IsNull() && totalCount.GetIntNumber() == 0))
        {
            Ctx_.AddSuggest("reminders__add_reminder_quasar");
        }

        if (answerJson.Has("canceled")) {
            Ctx_.AddSuggest("reminders__list_for_today");
            Ctx_.AddSuggest("reminders__list_for_tomorrow");
        }
    }

    bool TryCancel(const TRemindersListWrapper& listWrapper) override {
        auto onEachId = [this](TStringBuf id) {
            return TryCancelReminder(id);
        };
        return listWrapper.ForEach(onEachId);
    }

private:
    bool TryCancelReminder(TStringBuf id) {
        const auto response = RequestApi(
            Req_,
            [&id](NHttpFetcher::TRequest& r) {
                r.SetMethod("DELETE").AddCgiParam("id", id);
            }
        );

        if (!response) {
            return false;
        }

        const NSc::TValue& error = (*response)["error"];
        if (!error.IsNull()) {
            LOG(ERR) << "Error in response from reminders api (cancel): " << response << Endl;
            CANCEL_HANDLERS[1].Descr.SetDefaultError(Ctx_);
            return false;
        }

        return true;
    }
};

void RemindersV1ListHandler(TRequest& req, TContext& ctx) {
    TRemindersBackendV1{req, ctx}.ProcessList();
}

void RemindersV1CancelHandler(TRequest& req, TContext& ctx) {
    TRemindersBackendV1{req, ctx}.ProcessCancel();
}

} // namespace

void TakeDayPartIntoAccount(TContext& ctx) {
    static const THashMap<TStringBuf, i64> dayPartToDefaultHours = {
        {"night", 12},
        {"morning", 8},
        {"day", 1},
        {"evening", 6}
    };

    TSlot* const dayPartSlot = ctx.GetSlot(SLOT_NAME_DAY_PART, SLOT_TYPE_DAY_PART);
    if (IsSlotEmpty(dayPartSlot)) {
        return;
    }

    auto period = NAlarm::DayPartToPeriod(dayPartSlot->Value.GetString());
    if (!period) {
        return;
    }

    TSlot* const timeSlot = ctx.GetSlot(SLOT_NAME_TIME, SLOT_TYPE_TIME);
    if (IsSlotEmpty(timeSlot)) {
        auto defaultHours = dayPartToDefaultHours.FindPtr(dayPartSlot->Value.GetString());
        if (defaultHours) {
            NSc::TValue timeValue;
            timeValue["hours"].SetIntNumber(*defaultHours);
            timeValue["period"].SetString(*period);
            ctx.CreateSlot(SLOT_NAME_TIME, SLOT_TYPE_TIME, true /* optional */, timeValue);
        }
    } else {
        NAlarm::AdjustTimeValue(timeSlot->Value, dayPartSlot->Value);
    }

    TRequest::ClearSlot(dayPartSlot);
}

TMaybe<NDatetime::TCivilSecond> ProcessShootTimeValue(TContext& ctx, TRequest& req) {
    using namespace NAlice::NScenarios::NAlarm;

    TMaybe<NDatetime::TCivilSecond> shootTime;

    req.MoveSlotValueToAnswer(SLOT_NAME_DATE, SLOT_TYPE_DATE);
    req.MoveSlotValueToAnswer(SLOT_NAME_TIME, SLOT_TYPE_TIME);
    NSc::TValue& answerJson = req.AnswerSlot().Json();

    const NSc::TValue& value = answerJson[SLOT_NAME_TIME];
    if (value.IsNull()) {
        ctx.AddAttention(FAIL_NO_TIME);
        return Nothing();
    }

    const TMaybe<TDayTime> timeFromSlot = TDayTime::FromValue(value);
    if (!timeFromSlot) {
        LOG(ERR) << "Error creating TDayTime from time slot" << Endl;
    }
    if (!timeFromSlot || timeFromSlot->HasRelativeNegative()) {
        req.SetError(FAIL_INVALID_DATETIME);
        return Nothing();
    }

    const NDatetime::TCivilSecond curDateTime = req.CurrentTime();

    if (const auto& dateValue = answerJson[SLOT_NAME_DATE]; !dateValue.IsNull()) {
        TMaybe<TDate> dateFromSlot = TDate::FromValue(dateValue);
        if (dateFromSlot) {
            auto onError = [&req, &ctx](EReminderFailState state) {
                if (state == EReminderFailState::Incomplete) {
                    LOG(ERR) << "date slot contains incomplete date (without days)" << Endl;
                    ctx.AddAttention(FAIL_INCOMPLETE_DATE);
                }
                else {
                    req.SetError(state == EReminderFailState::InPast ? FAIL_PAST_DATETIME : FAIL_INVALID_DATETIME);
                }
                return false;
            };

            auto onSuccess = [&shootTime](NDatetime::TCivilSecond time) -> bool {
                shootTime.ConstructInPlace(time);
                return true;
            };

            if (!GetReminderDate<bool>(curDateTime, *dateFromSlot, timeFromSlot, onSuccess, onError)) {
                return Nothing();
            }
        } else {
            LOG(ERR) << "Error creating TDate from date slot" << Endl;
            req.SetError(FAIL_INVALID_DATETIME);
            return Nothing();
        }
    }

    if (!shootTime) {
        shootTime.ConstructInPlace(GetAlarmTime(curDateTime, *timeFromSlot));
    }
    else {
        shootTime.ConstructInPlace(GetEventTime(curDateTime, *shootTime, *timeFromSlot));
    }

    if (shootTime < curDateTime) {
        req.SetError(FAIL_PAST_DATETIME);
        return Nothing();
    }

    return shootTime;
}

void ReminderCreate(TContext& ctx) {
    SetReminderProductScenario(ctx);
    if (!TRequest::Handle(ctx, CREATE_HANDLERS)) {
        TryAddShowPromoDirective(ctx);
    }
}

void ReminderCancel(TContext& ctx) {
    SetReminderProductScenario(ctx);
    TRequest::Handle(ctx, CANCEL_HANDLERS);
}

void RemindersListCancel(TContext& ctx) {
    SetReminderProductScenario(ctx);
    TRequest::Handle(ctx, LIST_HANDLERS);
}

void ReminderLanding(TContext& ctx) {
    SetReminderProductScenario(ctx);
    Y_SCOPE_EXIT(&ctx) {
        ctx.AddStopListeningBlock();
    };

    const TFormsDescr& descr = DESCR_PUSH_LANDING;

    TMaybe<TRequest> request = TRequest::Create(ctx, descr);
    if (!request) {
        return;
    }

    const TSlot* const slotReminderId = ctx.GetSlot(SLOT_NAME_REMINDER_ID, SLOT_TYPE_REMINDER_ID);
    if (IsSlotEmpty(slotReminderId)) {
        LOG(ERR) << "Slot reminder_id is empty" << Endl;
        return request->SetDefaultError();
    }

    const TStringBuf id{slotReminderId->Value.GetString()};
    TMaybe<NSc::TValue> respJson{RequestApi(*request, [&id](NHttpFetcher::TRequest& r) { r.AddCgiParam("id", id); })};
    if (!respJson) {
        return;
    }

    const NSc::TValue& result = (*respJson)["result"];
    const NSc::TValue& error = (*respJson)["error"];
    if (!error.IsNull() || result.IsNull()) {
        LOG(ERR) << "Getting reminder item by id '" << id << "': " << respJson->ToJson() << Endl;
        return request->SetDefaultError();
    }

    NSc::TValue& answerJson = request->AnswerSlot().Json();
    answerJson["what"].SetString(result["name"].GetString());
    ConvertFromISO8601(result["reminderDate"].GetString(), request->CurrentTime(), request->UserTZ, &answerJson);
}

bool ReminderCleanupForTestUser(TContext& ctx) {
    TMaybe<TRequest> request = TRequest::Create(ctx, DESCR_REMOVE_ALL);
    if (!request) {
        return false;
    }

    const TMaybe<NSc::TValue> respJson = RequestApi(*request, [](NHttpFetcher::TRequest&) {});
    if (!respJson) {
        return false;
    }

    for (const NSc::TValue& i : (*respJson)["result"]["reminders"].GetArray()) {
        const TStringBuf id{i["id"].GetString()};
        const TMaybe<NSc::TValue> respJson = RequestApi(
            *request, [id](NHttpFetcher::TRequest& r) {
                r.SetMethod("DELETE").AddCgiParam("id", id);
            }
        );

        if (!respJson || (*respJson)["result"]["status"].GetString() != "ok") {
            return false;
        }
    }

    return true;
}

void RemindersListCancelStop(TContext& ctx) {
    SetReminderProductScenario(ctx);
    const TFormsDescr* descr = nullptr;

    if (TRequest::IsSearchAppBehavior(ctx.MetaClientInfo())) {
        descr = &DESCR_LIST_TEXT_AND_VOICE;
    } else {
        descr = &DESCR_LIST_VOICE;
    }

    TMaybe<TRequest> request = TRequest::Create(ctx, *descr);
    if (!request) {
        return;
    }

    DeleteAllCancelationConfirmationFlags(*request);
    DeleteTemporalFlags(*request);

    return TRequest::ListStopAction(ctx, *descr);
}

// TRemindersListScroll -------------------------------------------------------
TRemindersListScroll::TRemindersListScroll(TAnswerSlot* slot)
    : Offset(0)
    , PerPage(ITEMS_PER_PAGE_FIRST)
{
    const NSc::TValue scrollJson = slot->Json().Delete("scroll");
    if (scrollJson.IsNull()) {
        return;
    }

    Offset = scrollJson["offset"].GetIntNumber(0);
    PerPage = scrollJson["perpage"].GetIntNumber(ITEMS_PER_PAGE_FIRST);
    IterationKey.ConstructInPlace(scrollJson["ik"]);
}

void TRemindersListScroll::UpdateFromApi(const NSc::TValue& apiJson) {
    IterationKey.ConstructInPlace(apiJson["result"]["iterationKey"]);
}

void TRemindersListScroll::WriteToContext(TRequest* request) const {
    NSc::TValue& scrollJson = request->AnswerSlot().Json()["scroll"];
    scrollJson["offset"].SetIntNumber(Offset + PerPage);
    scrollJson["perpage"].SetIntNumber(PerPage == ITEMS_PER_PAGE_FIRST ? ITEMS_PER_PAGE_AFTER_FIRST : PerPage);
    if (IterationKey.Defined()) {
        scrollJson["ik"] = *IterationKey;
    }

    request->Ctx.CreateSlot(SLOT_NAME_OFFSET, SLOT_TYPE_NUM, true /* optional */)->Value.SetIntNumber(Offset);
}

// IRemindersBackend -------------------------------------------------------------
bool IRemindersBackend::TryFillDateIfOnlyTimeIsProvided() {
    auto& request = Req_;

    TSlot* const dateSlot = request.Ctx.GetSlot(SLOT_NAME_DATE, SLOT_TYPE_DATE);
    TSlot* const timeSlot = request.Ctx.GetSlot(SLOT_NAME_TIME, SLOT_TYPE_TIME);
    if (!IsSlotEmpty(dateSlot) || IsSlotEmpty(timeSlot)) {
        return true;
    }

    auto getResponse = GetRemindersList(/* soonestFirst */true, /* df */nullptr, /* scroll */nullptr);
    NSc::TValue normalizedTime = NormalizeTimeValue(request, timeSlot->Value);
    if (!getResponse || normalizedTime.IsNull()) {
        request.SetDefaultError();
        return false;
    }
    timeSlot->Value = normalizedTime;

    NSc::TValue matchedReminder = NSc::Null();
    for (const NSc::TValue& reminder : getResponse->List.GetArray()) {
        NSc::TValue tmp;
        if (!ConvertFromISO8601(reminder["reminderDate"].GetString(), request.CurrentTime(), request.UserTZ, &tmp)) {
            request.SetDefaultError();
            return false;
        }
        if (tmp["time"] == normalizedTime) {
            matchedReminder = tmp;
            break;
        }
    }

    if (!matchedReminder.IsNull()) {
        dateSlot->Value = matchedReminder["date"];
        return true;
    }

    return false;
}

TMaybe<IRemindersBackend::TRemindersResult> IRemindersBackend::GetReminders(bool soonestFirst,
                                                                            TRemindersListScroll* scroll)
{
    return GetRemindersList(soonestFirst, GetDateFilter(Req_).Get(), scroll);
}

void IRemindersBackend::ProcessList() {
    auto& answerJson = Req_.AnswerSlot().Json();
    bool previousAnswerWasUnauthorized = answerJson.TrySelect("type").GetString() == "authorization";

    if (answerJson.Has("canceled") || previousAnswerWasUnauthorized) {
        answerJson.Clear();
    } else {
        DeleteTemporalFlags(Req_);
    }

    Y_SCOPE_EXIT(this) {
        TRequest::ClearSlot(Req_.Ctx.GetSlot(SLOT_NAME_REMINDER_ID_TO_CANCEL));
        SuggestForListing();
    };

    if (IsCancellationRequired() && TryHandleCancellation()) {
        return;
    }

    DeleteAllCancelationConfirmationFlags(Req_);
    if (!IsCancellationRequired() && !IsSlotEmpty(Ctx_.GetSlot(SLOT_NAME_REMINDER_ID_TO_CANCEL))) {
        answerJson["id_without_cancelation"] = "yes";
        return;
    }

    TRemindersListScroll scroll(&Req_.AnswerSlot());

    answerJson.Clear();

    if (!scroll.HasMore()) {
        answerJson["is_finish"].SetString("yes");
        return;
    }

    if (IsSlotEmpty(Ctx_.GetSlot(SLOT_NAME_DATE)) && !IsSlotEmpty(Ctx_.GetSlot(SLOT_NAME_TIME))) {
        answerJson["soonest"] = "yes";
    }

    const auto reminders = GetReminders(true, &scroll);
    if (!reminders) {
        return;
    }

    const auto total = reminders->Total;

    TAnswerSlot& slot = Req_.AnswerSlot();

    slot.Json()["total_reminders_count"].SetIntNumber(total);
    slot.Json()["reminders"] = reminders->List;

    if (IsCancellationRequired()) {
        const bool wasASlotReset = WasAnswerSlotReset(Ctx_);
        if (total == 1 && wasASlotReset) {
            SetDefaultAction(Ctx_);
            if (TryCancel(TRemindersListWrapper{reminders->List, false})) {
                RecordSuccessfulCancelation(slot);
            }
            return;
        } else if (IsIdAllSpecified(Ctx_) && 
                   (IsFirstRequestWithinListOrCancelScenario(Ctx_) || wasASlotReset))
        {
            ConfirmAllCancelation(Req_);
        }
    }

    scroll.WriteToContext(&Req_);

    PostProcessList(*reminders);
}

void IRemindersBackend::ProcessCancel() {
    TAnswerSlot& slot = Req_.AnswerSlot();

    const TString id{slot.Json().Delete("id").GetString()};

    slot.Json().Clear()["is_set"].SetString("no");

    if (!id) {
        slot.Json()["is_set"].SetString("ellipsis");
        return;
    }

    NSc::TValue reminders;
    reminders.Push()["id"].SetString(id);
    if (TryCancel(TRemindersListWrapper{reminders, false})) {
        slot.Json()["is_set"].SetString("yes");
    }

    Ctx_.AddStopListeningBlock();
}

bool IRemindersBackend::TryHandleCancellation() {
    NSc::TValue& previousAnswer = Req_.AnswerSlot().Json();
    if (previousAnswer.IsNull()) {
        return false;
    }

    NSc::TValue& reminders = previousAnswer["reminders"];
    NSc::TValue& totalCount = previousAnswer["total_reminders_count"];

    TSlot* const numIdSlot = Req_->GetSlot(SLOT_NAME_REMINDER_ID_TO_CANCEL, SLOT_TYPE_NUM);
    if (!IsSlotEmpty(numIdSlot)) {
        i64 numId = numIdSlot->Value.GetIntNumber();
        DeleteAllCancelationConfirmationFlags(Req_);
        if (numId <= 0 || static_cast<size_t>(numId) > reminders.ArraySize()) {
            previousAnswer["invalid_id"].SetString("yes");
            return true;
        }

        if (reminders.ArraySize() > 1) {
            NSc::TValue newReminders;
            newReminders.SetArray();
            newReminders.Push(reminders.Get(numId - 1));
            std::swap(reminders, newReminders);
        }
        totalCount.SetIntNumber(1);
        Ctx_.GetOrCreateSlot(SLOT_NAME_OFFSET, SLOT_TYPE_NUM)->Value.SetIntNumber(0);
    }

    // Удали все напоминания, да!
    const bool isAllLocalCancelationConfirmed = IsAllLocalCancelationConfirmed(Req_);

    if (IsIdAllSpecified(Ctx_) &&
        !(IsAllCancelationConfirmed(Req_) || isAllLocalCancelationConfirmed))
    {
        ConfirmAllLocalCancelation(Req_);
        return true;
    }

    if (IsAllCancelationConfirmed(Req_) && totalCount.GetIntNumber() > ITEMS_PER_PAGE_MIN_MAX) {
        TRemindersListWrapper listWrapper{reminders, isAllLocalCancelationConfirmed};
        if (auto response = GetReminders(true, nullptr); response.Defined()) {
            reminders = std::move(response->List);
        } else {
            CANCEL_HANDLERS[1].Descr.SetDefaultError(Ctx_);
            return true;
        }
    }

    if (IsOneItemLeft(Req_)
        || IsAllCancelationConfirmed(Req_)
        || isAllLocalCancelationConfirmed)
    {
        SetDefaultAction(Ctx_);

        TRemindersListWrapper listWrapper{reminders, isAllLocalCancelationConfirmed};
        if (!TryCancel(listWrapper)) {
            return true;
        }

        totalCount.SetIntNumber(reminders.ArraySize());
        RecordSuccessfulCancelation(Req_.AnswerSlot());
    }

    return true;
}

bool IRemindersBackend::IsCancellationRequired() const {
    if (!IsCancellationSupported()) {
        return false;
    }

    const TSlot* actionSlot = Ctx_.GetSlot(SLOT_NAME_ACTION, SLOT_TYPE_CANCEL_ACTION);
    return !IsSlotEmpty(actionSlot) && actionSlot->Value.GetString() == SLOT_TYPE_CANCEL_ACTION;
}

} // namespace NReminders
} // namespace NBASS

#include "todo.h"
#include "request.h"

#include <alice/library/scenarios/alarm/date_time.h>
#include <alice/library/scenarios/alarm/helpers.h>

#include <alice/bass/forms/common/personal_data.h>
#include <alice/bass/forms/context/context.h>
#include <alice/bass/forms/directives.h>

#include <alice/bass/libs/logging_v2/logger.h>

#include <alice/library/analytics/common//product_scenarios.h>
#include <alice/library/network/headers.h>

#include <library/cpp/timezone_conversion/civil.h>

#include <util/charset/utf8.h>

/** Documentation:
 * full: https://wiki.yandex-team.ru/assistant/dialogs/todo/#opisaniescenarijaposmotretspisoknapominanijj
 * vins-bass: https://wiki.yandex-team.ru/assistant/dialogs/todo/Vins-Bass-protokol/
 * todo api: https://wiki.yandex-team.ru/calendar/api/ext/backend/todos/#poluchitspisoktodopoid (beware all ports not 81 but 80)
 */

namespace NBASS {
namespace NReminders {

namespace {

constexpr TStringBuf CANCEL_TODO_ELLIPSIS = "personal_assistant.scenarios.cancel_todo__ellipsis";

constexpr TStringBuf FAIL_UNSUPPORTED_DEVICE = "unsupported_device";
constexpr TStringBuf FAIL_PAST_DATETIME = "past_datetime";

constexpr TStringBuf SLOT_TYPE_CREATE = "create_todo_ans";
constexpr TStringBuf SLOT_TYPE_LIST = "list_todo_ans";
constexpr TStringBuf SLOT_NAME_WHAT = "what";
constexpr TStringBuf SLOT_TYPE_WHAT = "string";

constexpr TStringBuf DEFAULT_LIST_ID = "Yandex.Alice.SpecialLists.MyTodos";
constexpr TStringBuf DEFAULT_LIST_TITLE = "Мои дела";

constexpr TStringBuf USER_AGENT = "YaBass-1.0";

constexpr i64 ITEMS_PERPAGE_FIRST = 3;
constexpr i64 ITEMS_PERPAGE_AFTER_FIRST = 3;

const TFormsDescr DESCR_CANCEL {
    TError::EType::TODOERROR,
    SLOT_TYPE_CREATE,
    "todo",
    "cancel",
    "canceling_failed",
};

const TFormsDescr DESCR_CREATE {
    TError::EType::TODOERROR,
    SLOT_TYPE_CREATE,
    "todo",
    "ok",
    "creating_failed",
    TFormsDescr::TErrorPostworkCallback([](TContext& ctx) {
        ctx.AddOnboardingSuggest();
    })
};

const TFormsDescr DESCR_LIST_TEXT_AND_VOICE {
    TError::EType::TODOERROR,
    SLOT_TYPE_LIST,
    "todo",
    "textandvoice",
    "list_failed",
};

const TFormsDescr DESCR_LIST_VOICE {
    TError::EType::TODOERROR,
    SLOT_TYPE_LIST,
    "todo",
    "voice",
    "list_failed",
};

const TFormsDescr DESCR_REMOVE_ALL {
    TError::EType::TODOERROR,
    SLOT_TYPE_LIST,
    "todo",
    "voice",
    "remove_failed",
    Nothing(),
    true /* do not check for supported */
};

class TScroll {
public:
    i64 Offset;
    i64 PerPage;
    i64 PerPageFirst;
    TMaybe<NSc::TValue> IterationKey;

public:
    // not by const because we remove scroll json from slot value
    explicit TScroll(TAnswerSlot* slot, const int itemsPerPage, const int itemsPerPageFirst);

    void UpdateFromApi(const NSc::TValue& apiJson);

    void WriteToContext(TRequest* req) const;

    bool HasMore() const {
        return !IterationKey.Defined() || !IterationKey->IsNull() ;
    }
};

TScroll::TScroll(TAnswerSlot* slot, const int itemsPerPage, const int itemsPerPageFirst)
    : Offset(0)
    , PerPage(itemsPerPage)
    , PerPageFirst(itemsPerPageFirst)
{
    const NSc::TValue scrollJson = slot->Json().Delete("scroll");
    if (scrollJson.IsNull()) {
        return;
    }

    Offset = scrollJson["offset"].GetIntNumber(0);
    PerPage = scrollJson["perpage"].GetIntNumber(itemsPerPageFirst);
    IterationKey.ConstructInPlace(scrollJson["ik"]);
}

void TScroll::UpdateFromApi(const NSc::TValue& apiJson) {
    IterationKey.ConstructInPlace(apiJson["iteration-key"]);
}

void TScroll::WriteToContext(TRequest* request) const {
    NSc::TValue& scrollJson = request->AnswerSlot().Json()["scroll"];
    scrollJson["offset"].SetIntNumber(Offset + PerPage);
    scrollJson["perpage"].SetIntNumber(PerPage == PerPageFirst ? ITEMS_PERPAGE_AFTER_FIRST : PerPage);
    if (IterationKey.Defined()) {
        scrollJson["ik"] = *IterationKey;
    }

    request->Ctx.CreateSlot("offset", "num", true)->Value.SetIntNumber(Offset);
}

TMaybe<NSc::TValue> RequestApi(TRequest& req, TStringBuf path, TCgiParameters& cgi) {
    NHttpFetcher::TRequestPtr r;

    r = req.Ctx.GetSources().CalendarApi(path).Request();
    TPersonalDataHelper personalDataHelper(req.Ctx);
    TString userTicket;
    if (!personalDataHelper.GetTVM2UserTicket(userTicket)) {
        LOG(ERR) << "Fetch calendar api (" << path << "): failed to obtain tvm userTicket" << Endl;
    } else {
        r->AddHeader(NAlice::NNetwork::HEADER_X_YA_USER_TICKET, userTicket);
    }

    if (!r) {
        LOG(ERR) << "Fetch calendar api (" << path << "): request object is null" << Endl;
        req.Descr().SetDefaultError(req.Ctx);
        return Nothing();
    }

    r->AddCgiParams(cgi).AddHeader("user-agent", USER_AGENT);

    NHttpFetcher::THandle::TRef h = r->Fetch();
    if (!h) {
        LOG(ERR) << "Fetch calendar api (" << path << "): handle object is null" << Endl;
        req.Descr().SetDefaultError(req.Ctx);
        return Nothing();
    }

    NHttpFetcher::TResponse::TRef resp = h->Wait();
    if (!resp) {
        LOG(ERR) << "Fetch calendar api (" << path << "): response object is null" << Endl;
        req.Descr().SetDefaultError(req.Ctx);
        return Nothing();
    }

    if (resp->IsError()) {
        LOG(ERR) << "Fetch calendar api (" << path << "): " << resp->GetErrorText() << Endl;
        req.Descr().SetDefaultError(req.Ctx);
        return Nothing();
    }

    try {
        return NSc::TValue::FromJsonThrow(resp->Data);
    } catch (const NSc::TSchemeParseException& e) {
        LOG(ERR) << "ParseJsonCalendarApi (" << path << "): " << e.Offset << ", " << e.Reason << ": " << resp->Data << Endl;
        req.Descr().SetDefaultError(req.Ctx);
        return Nothing();
    }

    return Nothing();
}

TMaybe<NSc::TValue> RequestListApi(TRequest& req, TScroll* scroll) {
    Y_ASSERT(scroll != nullptr);

    TCgiParameters cgi;
    cgi.InsertUnescaped("only-not-completed", "true");
    cgi.InsertUnescaped("soonest_first", "true");
    cgi.InsertUnescaped("uid", req.Uid);
    cgi.InsertUnescaped("count", ToString(scroll->PerPage));
    if (scroll->IterationKey && !scroll->IterationKey->IsNull()) {
        cgi.InsertUnescaped("iteration-key", scroll->IterationKey->GetString());
    }

    if (const TDateSlot* dateSlot = req.DateSlot()) {
        cgi.InsertUnescaped("due-from", dateSlot->Format("%E4Y-%m-%d"));
        cgi.InsertUnescaped("due-to", dateSlot->Format("%E4Y-%m-%dT23:59:59"));
    } else {
        cgi.InsertUnescaped("undue-first", "false");
    }

    TMaybe<NSc::TValue> respJson = RequestApi(req, TStringBuf("/api/get-todo-items.json"), cgi);
    if (!respJson) {
        return Nothing();
    }
    scroll->UpdateFromApi(*respJson);

    return respJson;
}

bool RequestCreateListApi(TRequest& req) {
    TCgiParameters cgi;
    cgi.InsertUnescaped("title", DEFAULT_LIST_TITLE);
    cgi.InsertUnescaped("list-external-id", DEFAULT_LIST_ID);
    cgi.InsertUnescaped("uid", req.Uid);

    TMaybe<NSc::TValue> respJson = RequestApi(req, "/api/create-todo-list.json", cgi);
    if (!respJson.Defined()) {
        return false;
    }

    const TStringBuf createdId = (*respJson)["todo-list-id"].GetString();
    if (!createdId) {
        LOG(ERR) << "Calendar API: unable to find new list id in response: " << respJson << Endl;
        req.SetDefaultError();
        return false;
    }

    return true;
}

TMaybe<NSc::TValue> RequestCreateApi(TRequest& req, TStringBuf what, bool firstRun = true) {
    TCgiParameters cgi;
    cgi.InsertUnescaped("title", what);
    cgi.InsertUnescaped("uid", req.Uid);
    cgi.InsertUnescaped("list-external-id", DEFAULT_LIST_ID);
    cgi.InsertUnescaped("fail-if-no-list", "true");
    if (const TDateSlot* dateSlot = req.DateSlot()) {
        cgi.InsertUnescaped("due-date", dateSlot->Format("%E4Y-%m-%d"));
    }

    TMaybe<NSc::TValue> respJson = RequestApi(req, "/api/create-todo-item.json", cgi);
    if (!respJson) {
        return Nothing();
    }

    const NSc::TValue& error = (*respJson)["error"];
    if (!error.IsNull()) {
        if (error["name"].GetString() == TStringBuf("todo-list-not-found")) {
            if (RequestCreateListApi(req)) {
                if (firstRun) {
                    return RequestCreateApi(req, what, false /* first run */);
                }
            }
            return Nothing();
        } else {
            LOG(ERR) << "Calendar api returns error (create): " << respJson << Endl;
            req.SetDefaultError();
            return Nothing();
        }
    }

    return respJson;
}

TString CreateUserListUrl(TStringBuf tz) {
    static constexpr TStringBuf host = "https://calendar.yandex.ru/todo";
    TCgiParameters cgi;
    cgi.InsertUnescaped("client", "alice");
    cgi.InsertUnescaped("tz", tz);
    cgi.InsertUnescaped("reminders", "1");
    return TStringBuilder() << host << '?' << cgi.Print();
}

void AddUrlSuggest(TContext& ctx, TStringBuf type, TStringBuf url) {
    NSc::TValue dataSuggest;
    dataSuggest["url"].SetString(url);
    ctx.AddSuggest(type, std::move(dataSuggest));
}

void SetTodoProductScenario(TContext& ctx) {
    ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::TODO);
}

} // namespace

void ApiCancel(TContext& ctx, const TStringBuf id) {
    const TFormsDescr& descr = DESCR_CANCEL;
    TMaybe<TRequest> request = TRequest::Create(ctx, descr);
    if (!request) {
        return;
    }

    TCgiParameters cgi;
    cgi.InsertUnescaped("uid", request->Uid);
    cgi.InsertUnescaped("todo-ids", id);
    const TMaybe<NSc::TValue> respJson = RequestApi(*request, "/api/delete-todo-items.json", cgi);
    if (!respJson) {
        return;
    }

    const NSc::TValue& errorJson = (*respJson)["error"];
    if (!errorJson.IsNull()) {
        LOG(ERR) << "Error in response from calendar api (cancel): " << respJson << Endl;
        return descr.SetDefaultError(ctx);
    }
}

void TodoCancel(TContext& ctx) {
    SetTodoProductScenario(ctx);
    ctx.CreateSlot("no_open_uri", "bool", true, true);
    if (ctx.FormName() != CANCEL_TODO_ELLIPSIS) {
        TodosList(ctx);
    }
    TSlot* listSlot = ctx.GetSlot("answer", "list_todo_ans");
    const int listSize = listSlot->Value["total_todo_count"];
    if (IsSlotEmpty(listSlot) || !listSize) {
        ctx.AddAttention("empty_todo_list");
        return;
    }

    TSlot* idSlot = ctx.GetSlot("id", "selection");

    if (!IsSlotEmpty(idSlot) && idSlot->Value.GetString() == "all") {
        TSlot* dateSlot = ctx.GetSlot("cancel_when", "date");
        if (IsSlotEmpty(dateSlot)) {
            ctx.AddAttention("cancel_all");
            TodoCleanupForTestUser(ctx);
        } else {
            const auto list = listSlot->Value;
            int deletedCount = 0;
            for (int i = 0; i < listSize; ++i) {
                auto item = list["todo"][i];
                if (item["date"].IsNull() || item["date"] != dateSlot->Value) {
                    continue;
                }
                const TStringBuf id = listSlot->Value["todo"][i]["id"].GetString();
                ApiCancel(ctx, id);
                deletedCount++;
            }
            if (deletedCount == 0) {
                ctx.AddAttention("no_todo_for_date");
            }
        }
        return;
    }

    idSlot = ctx.GetSlot("id", "num");
    if (IsSlotEmpty(idSlot) && listSize > 1) {
        idSlot->Optional = false;
        return;
    }

    if (listSize == 1) {
        ctx.AddAttention("single_todo");
    }

    const int numberInList = listSize == 1 ? 0 : idSlot->Value.GetIntNumber() - 1;
    if (numberInList < 0 || numberInList >= listSize) {
        idSlot->Reset();
        idSlot->Optional = false;
        ctx.AddAttention("invalid_id");
        return;
    }

    const TStringBuf id = listSlot->Value["todo"][numberInList]["id"].GetString();
    ApiCancel(ctx, id);
}

void TodoCancelLast(TContext& ctx) {
    SetTodoProductScenario(ctx);
    const TFormsDescr& descr = DESCR_CANCEL;

    if (!TRequest::IsSearchAppBehavior(ctx.MetaClientInfo()) && !ctx.MetaClientInfo().IsSmartSpeaker()) {
        return descr.SetError(ctx, FAIL_UNSUPPORTED_DEVICE);
    }

    TMaybe<TRequest> request = TRequest::Create(ctx, descr);
    if (!request) {
        return;
    }

    TAnswerSlot& slot = request->AnswerSlot();

    const TString id{slot.Json().Delete("id").GetString()};

    slot.Json().Clear()["is_set"].SetString("no");

    if (!id) {
        const TSlot* const whatSlot = ctx.GetSlot(SLOT_NAME_WHAT);
        if (IsSlotEmpty(whatSlot)) {
            slot.Json()["is_set"].SetString("ellipsis");
            return;
        }
        return;
    }

    TCgiParameters cgi;
    cgi.InsertUnescaped("uid", request->Uid);
    cgi.InsertUnescaped("todo-ids", id);

    const TMaybe<NSc::TValue> respJson = RequestApi(*request, "/api/delete-todo-items.json", cgi);
    if (!respJson) {
        return;
    }

    const NSc::TValue& errorJson = (*respJson)["error"];
    if (!errorJson.IsNull()) {
        LOG(ERR) << "Error in response from calendar api (cancel): " << respJson << Endl;
        return descr.SetDefaultError(ctx);
    }

    slot.Json()["is_set"].SetString("yes");
}

void TodoCreate(TContext& ctx) {
    SetTodoProductScenario(ctx);
    const TFormsDescr& descr = DESCR_CREATE;

    if (!TRequest::IsSearchAppBehavior(ctx.MetaClientInfo()) && !ctx.MetaClientInfo().IsSmartSpeaker()) {
        return descr.SetError(ctx, FAIL_UNSUPPORTED_DEVICE);
    }

    TMaybe<TRequest> request = TRequest::Create(ctx, descr);
    if (!request) {
        return;
    }

    if (TDateSlot* dateSlot = request->DateSlot()) {
        if (dateSlot->InPast(NDatetime::TCivilDay{request->CurrentTime()})) {
            dateSlot->Clear();
            return request->SetErrorWithoutPostwork(FAIL_PAST_DATETIME);
        }
    }

    TSlot* const whatSlot = ctx.GetOrCreateSlot(SLOT_NAME_WHAT, SLOT_TYPE_WHAT);
    if (IsSlotEmpty(whatSlot)) {
        whatSlot->Optional = false;
        return;
    }

    request->AnswerSlot().Json()["is_set"].SetString("no");

    const TStringBuf whatValue = whatSlot->Value.GetString();
    if (whatValue.size() < 1 || GetNumberOfUTF8Chars(whatValue) > 1024) {
        LOG(ERR) << "'what' size constraints failed" << Endl;
        return descr.SetDefaultError(ctx);
    }

    const TMaybe<NSc::TValue> respJson = RequestCreateApi(*request, whatValue);
    if (!respJson) {
        return;
    }

    const TString createdId = (*respJson)["todo-item"]["id"].ForceString();
    if (createdId.empty()) {
        LOG(ERR) << "Unable to add todo (id not found in response): " << *respJson << Endl;
        return descr.SetDefaultError(ctx);
    }

    request->AnswerSlot().Json()["id"].SetString(createdId);
    request->AnswerSlot().Json()["is_set"].SetString("yes");

    AddUrlSuggest(ctx, "todo__show_todo_list", CreateUserListUrl(ctx.Meta().TimeZone()));

    // cancel suggest
    TContext formUpdate(ctx, TODO_FORM_NAME_CANCEL);
    NSc::TValue& json = DESCR_CANCEL.CreateSlotInContext(formUpdate);
    json["id"].SetString(createdId);
    json["type"].SetString(DESCR_CANCEL.DefaultType);
    ctx.AddSuggest(TStringBuf("todo__cancel"),
                   NSc::Null(),
                   formUpdate.ToJson(
                       TContext::EJsonOut::TopLevel
                       | TContext::EJsonOut::Resubmit
                   )
    );

    ctx.AddSuggest("todo__add_todo_for_today");
    ctx.AddSuggest("todo__add_todo_for_tomorrow");

    if (ctx.MetaClientInfo().IsSmartSpeaker()) {
        ctx.AddStopListeningBlock();
    }
}

void TodosListStop(TContext& ctx) {
    SetTodoProductScenario(ctx);
    const TFormsDescr* descr = nullptr;

    if (TRequest::IsSearchAppBehavior(ctx.MetaClientInfo())) {
        descr = &DESCR_LIST_TEXT_AND_VOICE;
    } else if (ctx.MetaClientInfo().IsSmartSpeaker()) {
        descr = &DESCR_LIST_VOICE;
    } else {
        return DESCR_LIST_VOICE.SetError(ctx, FAIL_UNSUPPORTED_DEVICE);
    }

    Y_ASSERT(descr != nullptr);

    return TRequest::ListStopAction(ctx, *descr);
}

void TodosList(TContext& ctx) {
    SetTodoProductScenario(ctx);
    const TFormsDescr* descr = nullptr;
    bool needAllListWithoutOpenUri = ctx.GetOrCreateSlot("no_open_uri", "bool")->Value.GetBool();

    if (TRequest::IsSearchAppBehavior(ctx.MetaClientInfo())) {
        descr = &DESCR_LIST_TEXT_AND_VOICE;
    } else {
        descr = &DESCR_LIST_VOICE;
    }

    TMaybe<TRequest> request = TRequest::Create(ctx, *descr);
    if (!request) {
        return;
    }

    const int itemsPerPageFirst = (needAllListWithoutOpenUri ? 0 : ITEMS_PERPAGE_FIRST);
    const int itemsPerPage = (needAllListWithoutOpenUri ? 100 : ITEMS_PERPAGE_FIRST);
    TScroll scroll(&request->AnswerSlot(), itemsPerPage, itemsPerPageFirst);

    request->AnswerSlot().Json().Clear();

    if (!scroll.HasMore()) {
        request->AnswerSlot().Json()["is_finish"].SetString("yes");
        return;
    }

    NSc::TValue& list = request->AnswerSlot().Json()["todo"].SetArray();
    NSc::TValue& totalJson = request->AnswerSlot().Json()["total_todo_count"].SetIntNumber(0);

    TMaybe<NSc::TValue> respJson = RequestListApi(*request, &scroll);
    if (!respJson) {
        return;
    }

    TMaybe<NDatetime::TTimeZone> tz;
    try {
        tz.ConstructInPlace(NDatetime::GetTimeZone((*respJson)["tz"].GetString("bad_timezone_holder")));
    } catch (const NDatetime::TInvalidTimezone& e) {
        LOG(ERR) << "Invalid time zone: '" << (*respJson)["tz"].GetString() << "' " << e.what() << Endl;
        return request->SetDefaultError();
    }

    Y_ASSERT(tz.Defined());

    for (const NSc::TValue& item : (*respJson)["todo-items"].GetArray()) {
        NSc::TValue& outItem = list.Push();

        const i64 dueTs = item["due-ts"].ForceIntNumber(-1);
        if (dueTs > 0) {
            NDatetime::TCivilDay day{NDatetime::Convert(TInstant::Seconds(dueTs), *tz)};

            NSc::TValue& dateJson = outItem["date"] = NAlice::NScenarios::NAlarm::DateToValue(request->CurrentTime(), day);
            // TODO move this logic to DateToValue() -> TDate::ToValue() via enum flags
            if (request->CurrentTime().year() == day.year()) {
                dateJson.Delete("years");
            }
        }

        outItem["what"] = item["title"];
        outItem["id"] = item["external-id"];
    }

    totalJson.SetIntNumber((*respJson)["total-count"].GetIntNumber(list.ArraySize()));

    scroll.WriteToContext(request.Get());

    const TString webViewUrl{CreateUserListUrl(ctx.Meta().TimeZone())};
    AddUrlSuggest(ctx, "todo__open_uri", webViewUrl);
    if (!needAllListWithoutOpenUri && TRequest::IsSearchAppBehavior(ctx.MetaClientInfo())) {
        NSc::TValue payload;
        payload["uri"].SetString(webViewUrl);
        ctx.AddCommand<TToDoShowReminderDirective>(TStringBuf("open_uri"), std::move(payload));
    } else if (ctx.MetaClientInfo().IsSmartSpeaker()) {
        ctx.AddSuggest("todo__add_todo_quasar");
        ctx.AddSuggest("reminders__reminders_list_quasar");
    }
}

bool TodoCleanupForTestUser(TContext& ctx) {
    TMaybe<TRequest> request = TRequest::Create(ctx, DESCR_REMOVE_ALL);
    if (!request) {
        return false;
    }

    TStringBuilder ids;
    {
        TCgiParameters cgi;
        cgi.InsertUnescaped("uid", request->Uid);

        const TMaybe<NSc::TValue> respJson{RequestApi(*request, TStringBuf("/api/get-todo-items.json"), cgi)};
        if (!respJson) {
            return false;
        }

        for (const NSc::TValue& i : (*respJson)["todo-items"].GetArray()) {
            if (ids) {
                ids << ',';
            }
            ids << i["id"].ForceString();
        }
    }

    if (!ids) {
        return true;
    }

    TCgiParameters cgi;
    cgi.InsertUnescaped("todo-ids", ids);
    cgi.InsertUnescaped("uid", request->Uid);
    const TMaybe<NSc::TValue> respJson{RequestApi(*request, TStringBuf("/api/delete-todo-items.json"), cgi)};
    if (!respJson) {
        return false;
    }

    return true;
}

} // namespace NReminders
} // namespace NBASS

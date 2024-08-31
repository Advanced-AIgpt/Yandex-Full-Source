#include "request.h"

#include "constants.h"
#include "helpers.h"

#include <alice/library/scenarios/alarm/date_time.h>
#include <alice/library/scenarios/alarm/helpers.h>

#include <alice/bass/forms/common/personal_data.h>
#include <alice/bass/forms/context/context.h>
#include <alice/bass/forms/urls_builder.h>

#include <alice/bass/libs/client/client_info.h>
#include <alice/bass/libs/logging_v2/logger.h>

#include <util/generic/hash.h>
#include <util/generic/strbuf.h>
#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>

namespace NBASS::NReminders {
namespace {

constexpr TStringBuf FAIL_UNSUPPORTED_DEVICE = "unsupported_device";

void AppendAuthRequest(TContext& ctx, const TFormsDescr& descr) {
    descr.CreateSlotInContext(ctx)["type"].SetString("authorization");

    const TString uri = GenerateAuthorizationUri(ctx);
    if (!uri.empty()) {
        NSc::TValue suggestJson;
        suggestJson["url"].SetString(uri);
        ctx.AddSuggest(TString::Join(descr.Prefix, "__authorization"), std::move(suggestJson));
    }
}

bool WasSlotFillingRequested(TRequest& request, const TStringBuf slotName) {
    const NSc::TValue& result = request.AnswerSlot().Json().TrySelect("requested").TrySelect(slotName);
    return result.IsBool() && result.GetBool();
}

void RequestSlotFilling(TRequest& request, const TStringBuf slotName, const TStringBuf slotType) {
    TSlot* const slot = request->GetOrCreateSlot(slotName, slotType);
    slot->Optional = false;
    request.AnswerSlot().Json()["requested"][slotName].SetBool(true);
}

TString ObtainPassportUID(TContext& ctx) {
    if (ctx.Meta().HasUID()) {
        return ToString(ctx.Meta().UID());
    }

    if (!ctx.IsAuthorizedUser()) {
        return {};
    }

    TPersonalDataHelper pdh{ctx};
    TString uid;
    if (!pdh.GetUid(uid)) {
        return {};
    }

    return uid;
}

} // namespace

// static
bool TRequest::IsSearchAppBehavior(const TClientInfo& ci) {
    // Right now everything what is not quasar has searchapp behavior.
    return !ci.IsSmartSpeaker();
}

TRequest::TRequest(TContext& ctx, TString uid, NDatetime::TTimeZone tz, const TFormsDescr& descr)
    : Ctx{ctx}
    , Uid{std::move(uid)}
    , UserTZ{tz}
    , DescrRef{descr}
    , CurrentDateTime{NDatetime::Convert(GetCurrentTimestamp(Ctx), UserTZ)}
    , CurrentEpoch{GetCurrentTimestamp(Ctx)}
{
}

// static
TMaybe<TRequest> TRequest::Create(TContext& ctx, const TFormsDescr& descr) {
    if (!descr.DoNotCheckForSupported && !ctx.ClientFeatures().SupportsRemindersAndTodos()) {
        descr.SetError(ctx, FAIL_UNSUPPORTED_DEVICE);
        return Nothing();
    }

    TString uid{ObtainPassportUID(ctx)};
    if (!uid) {
        AppendAuthRequest(ctx, descr);
        return Nothing();
    }

    try {
        NDatetime::TTimeZone tz = NDatetime::GetTimeZone(ctx.UserTimeZone());
        return TRequest{ctx, uid, tz, descr};
    } catch (const NDatetime::TInvalidTimezone& e) {
        LOG(ERR) << "Invalid time zone: " << ctx.UserTimeZone() << ' ' << e.what() << Endl;
        ctx.AddErrorBlock(TError{TError::EType::SYSTEM}, "time_zone");
    }

    return Nothing();
}

const TRequest::THandler* TRequest::Handle(TContext& ctx, const TVector<THandler>& handlers) {
    const THandler* handler = nullptr;
    for (const auto& h : handlers) {
        if (h.CheckIfSuitable(ctx)) {
            handler = &h;
            break;
        }
    }

    if (handler) {
        try {
            TString uid;
            if (handler->Descr.IsUidRequired) {
                uid = ObtainPassportUID(ctx);
                if (!uid) {
                    AppendAuthRequest(ctx, handler->Descr);
                    return nullptr;
                }
            }

            NDatetime::TTimeZone tz = NDatetime::GetTimeZone(ctx.UserTimeZone());
            TRequest req{ctx, std::move(uid), tz, handler->Descr};
            handler->Handle(req, ctx);
        } catch (const NDatetime::TInvalidTimezone& e) {
            LOG(ERR) << "Invalid time zone: " << ctx.UserTimeZone() << ' ' << e.what() << Endl;
            ctx.AddErrorBlock(TError{TError::EType::SYSTEM}, "time_zone");
        }
    }

    return handler;
}

TAnswerSlot& TRequest::AnswerSlot() {
    if (!AnswerSlotHolder.Defined()) {
        AnswerSlotHolder.ConstructInPlace(*this);
    }
    return *AnswerSlotHolder;
}

TDateSlot* TRequest::DateSlot() {
    if (!DateSlotHolder.Defined()) {
        // XXX creates every call since Create() could return 'undef maybe'
        DateSlotHolder = TDateSlot::Create(*this);
    }

    return DateSlotHolder.Get();
}

NDatetime::TCivilSecond TRequest::CurrentTime() const {
    return CurrentDateTime;
}

// static
TMaybe<TDateSlot> TDateSlot::Create(const TRequest& req) {
    TSlot* const dateSlot = req.Ctx.GetSlot("date" /* name */, "date" /* type */);
    if (IsSlotEmpty(dateSlot)) {
        return Nothing();
    }

    const TMaybe<NAlice::NScenarios::NAlarm::TDate> date = NAlice::NScenarios::NAlarm::TDate::FromValue(dateSlot->Value);
    if (!date || !date->HasExactDay()) {
        LOG(ERR) << "Unable to create date object from date slot" << Endl;
        return Nothing();
    }

    const NDatetime::TCivilDay shootTime{date->Apply(req.CurrentTime())};

    req.Ctx.CreateSlot("date" /* name */, "date" /* type */, true /* optional */)->Value = NAlice::NScenarios::NAlarm::DateToValue(req.CurrentTime(), shootTime);

    return TDateSlot{req.UserTZ, shootTime, dateSlot};
}

bool TDateSlot::InPast(const NDatetime::TCivilDay& curDay) const {
    return Day < curDay;
}

TAnswerSlot::TAnswerSlot(TRequest& req)
    : Ctx{req.Ctx}
    , DescrRef{req.Descr()}
    , JsonRef{nullptr}
{
}

TAnswerSlot::~TAnswerSlot() {
    NSc::TValue& type = Json()["type"];
    if (type.IsNull()) {
        type.SetString(DescrRef.DefaultType);
    }
}

TAnswerSlot& TAnswerSlot::SetDefaultType() {
    Json()["type"].SetString(DescrRef.DefaultType);
    return *this;
}

NSc::TValue& TAnswerSlot::Json() {
    if (!JsonRef) {
        JsonRef = &Ctx.GetOrCreateSlot(SlotName(), DescrRef.SlotType)->Value;
    }

    return *JsonRef;
}

void TFormsDescr::SetDefaultError(TContext& ctx) const {
    SetError(ctx, DefaultErrorCode);
}

void TFormsDescr::SetError(TContext& ctx, TStringBuf errCode) const {
    SetErrorWithoutPostwork(ctx, errCode);

    if (PostworkErrorCallback) {
        (*PostworkErrorCallback)(ctx);
    }
}

void TFormsDescr::SetErrorWithoutPostwork(TContext& ctx, TStringBuf errCode) const {
    NSc::TValue json;
    json["code"].SetString(errCode);
    ctx.AddErrorBlock(TError{ErrorType}, std::move(json));
}

NSc::TValue& TFormsDescr::CreateSlotInContext(TContext& ctx) const {
    // XXX move it under TAnswerSlot it is his/her logic
    return ctx.CreateSlot(TAnswerSlot::SlotName(), SlotType, true)->Value;
}

TString TDateSlot::Format(TStringBuf format) const {
    return NDatetime::Format(format, Day, TZ);
}

void TDateSlot::Clear() {
    Slot->Value.Clear();
}

// static
void TRequest::ListStopAction(TContext& ctx, const TFormsDescr& descr) {
    TMaybe<TRequest> request = TRequest::Create(ctx, descr);
    if (!request) {
        return;
    }

    TAnswerSlot& slot = request->AnswerSlot();

    NSc::TValue newData;
    for (TStringBuf saveSlotName : { TStringBuf("scroll") }) {
        newData[saveSlotName] = slot.Json()[saveSlotName];
    }
    slot.Json() = std::move(newData);
}

void TRequest::MoveSlotValueToAnswer(const TStringBuf slotName, const TStringBuf slotType) {
    TSlot* const slot = Ctx.GetSlot(slotName, slotType);
    NSc::TValue& destination = AnswerSlot().Json()[slotName];
    if (!IsSlotEmpty(slot) && destination.IsNull()) {
        destination = std::move(slot->Value);
    }
    ClearSlot(slot);
}

void TRequest::ClearSlot(const TStringBuf slotName, const TStringBuf slotType) {
    ClearSlot(Ctx.GetSlot(slotName, slotType));
}

// static
void TRequest::ClearSlot(TSlot* slot) {
    if (slot) {
        slot->Value.SetNull();
        slot->Optional = true;
    }
}

void TRequest::CopySlotValueFromAnswer(const TStringBuf slotName, const TStringBuf slotType) {
    const NSc::TValue& source = AnswerSlot().Json().TrySelect(slotName);
    if (!source.IsNull()) {
        Ctx.CreateSlot(slotName, slotType, true /* optional */, source);
    }
}

TMaybe<TString> TRequest::ObtainRemindText() {
    TMaybe<TString> text;
    if (const TSlot* whatSlot = Ctx.GetSlot(SLOT_NAME_WHAT, SLOT_TYPE_WHAT); IsSlotEmpty(whatSlot)) {
        if (const auto value = ProcessEmptySlotWhatValue(); value.Defined()) {
            text = *value;
        }
    } else if (!whatSlot->Value.GetString().Empty()) {
        text = whatSlot->Value.GetString();
    }

    return text;
}

TMaybe<TStringBuf> TRequest::ProcessEmptySlotWhatValue() {
    if (!WasSlotFillingRequested(*this, SLOT_NAME_WHAT)) {
        MoveSlotValueToAnswer(SLOT_NAME_DATE, SLOT_TYPE_DATE);
        MoveSlotValueToAnswer(SLOT_NAME_TIME, SLOT_TYPE_TIME);
        RequestSlotFilling(*this, SLOT_NAME_WHAT, SLOT_TYPE_WHAT);
        return Nothing();
    }

    ClearSlot(SLOT_NAME_DATE, SLOT_TYPE_DATE);
    ClearSlot(SLOT_NAME_TIME, SLOT_TYPE_TIME);
    return Ctx.Meta().Utterance();
}

} // namespace NBASS::NReminders

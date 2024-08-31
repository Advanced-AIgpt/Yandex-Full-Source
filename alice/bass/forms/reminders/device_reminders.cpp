#include "device_reminders.h"
#include "helpers.h"
#include "reminder.h"

#include <alice/bass/forms/context/context.h>
#include <alice/bass/forms/directives.h>
#include <alice/bass/libs/client/experimental_flags.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/library/analytics/common/product_scenarios.h>

#include <alice/library/proto/protobuf.h>
#include <alice/megamind/protos/scenarios/directives.pb.h>

#include <alice/library/scenarios/reminders/api.h>

#include <alice/megamind/protos/common/permissions.pb.h>

#include <util/generic/guid.h>

namespace NBASS::NReminders {
namespace {

struct TCallbackDefinition {
    TStringBuf Name;
    TStringBuf KeyName;
    TStringBuf Type;
    bool Success;

    void PutInto(NSc::TValue& dst, const NSc::TValue& aux) const {
        auto& callback = dst[KeyName];
        callback["name"].SetString(Name);
        callback["type"].SetString("server_action");

        auto& payload = callback["payload"];
        payload.MergeUpdate(aux);
        payload["type"].SetString(Type);
        payload["success"].SetBool(Success);
    }
};

const TCallbackDefinition CREATION_SUCCESS_CALLBACK = { "reminders_on_success_callback", "on_success_callback", "Creation", true };
const TCallbackDefinition CREATION_FAIL_CALLBACK = { "reminders_on_fail_callback", "on_fail_callback", "Creation", false };
const TCallbackDefinition CANCELLATION_SUCCESS_CALLBACK = { "reminders_on_success_callback", "on_success_callback", "Cancelation", true};
const TCallbackDefinition CANCELLATION_FAIL_CALLBACK = { "reminders_on_fail_callback", "on_fail_callback", "Cancelation", false };

TString SeriazlizeToCallback(const TString& name, const google::protobuf::Message& proto) {
    NAlice::NScenarios::TCallbackDirective d;
    d.SetName(name);
    *d.MutablePayload() = std::move(NAlice::MessageToStruct(proto));
    return NAlice::ProtoToBase64String(d);
}

class TRemindersBackendLocal : public IRemindersBackend {
public:
    using IRemindersBackend::IRemindersBackend;

protected:
    TMaybe<TRemindersResult> GetRemindersList(bool /* soonestFirst */,
                                              const TDateBounds* dateBounds,
                                              TRemindersListScroll* scroll) override
    {
        const auto& srcReminders = Ctx_.Meta().DeviceState().DeviceReminders().List();

        NSc::TValue reminders;
        reminders.SetArray();
        for (const auto& reminder : srcReminders) {
            TInstant shootAt;
            if (time_t epoch = 0; TryFromString(reminder.Epoch(), epoch)) {
                shootAt = TInstant::Seconds(epoch);
            } else {
                LOG(ERR) << "Unable to convert '" << reminder.Epoch() << "' to time_t" << Endl;
                Req_.SetDefaultError();
                return Nothing();
            }

            if (dateBounds && !dateBounds->IsIn(shootAt)) {
                continue;
            }

            NSc::TValue& dst = reminders.Push();
            dst["id"] = reminder.Id();
            dst["what"] = reminder.Text();
            dst["origin"].SetString(Ctx_.MetaClientInfo().DeviceId);
            if (!CreateDateTime(shootAt, Req_.CurrentTime(), Req_.UserTZ, dst)) {
                Req_.SetDefaultError();
                return Nothing();
            }
        }

        if (scroll) {
            scroll->PerPage = reminders.ArraySize();
        }

        return TRemindersResult{std::move(reminders), static_cast<i64>(reminders.ArraySize())};
    }

    bool TryCancel(const TRemindersListWrapper& listWrapper) override {
        NSc::TValue directive;
        NSc::TValue aux;

        if (listWrapper.IsEverything()) {
            directive["action"].SetString("all");
            aux["action"].SetString("all");
        } else {
            THashMap<TString, std::reference_wrapper<const NSc::TValue>> deviceReminders;
            for (const auto& reminder : Ctx_.Meta().DeviceState().DeviceReminders().List()) {
                deviceReminders.emplace(reminder.Id(), *reminder.GetRawValue());
            }

            directive["action"].SetString("id");
            auto& ids = directive["id"];
            aux["action"].SetString("id");

            NSc::TValue reminders;
            auto onEachId = [&reminders, &ids, &deviceReminders](TStringBuf id) {
                if (const auto* reminder = deviceReminders.FindPtr(id)) {
                    reminders.Push(reminder->get());
                }
                ids.Push(id);
                return true;
            };
            listWrapper.ForEach(onEachId);

            if (ids.ArraySize() < 1) {
                return false;
            }

            if (reminders.ArraySize() != 0) {
                aux["reminders"] = std::move(reminders);
            }

            aux["ids"] = ids;
        }

        CANCELLATION_SUCCESS_CALLBACK.PutInto(directive, aux);
        CANCELLATION_FAIL_CALLBACK.PutInto(directive, aux);

        Ctx_.AddCommand<TRemindersCancelDirective>("reminders_cancel_directive", std::move(directive));

        // This type is for vins to prevent say something and create nlg answer.
        Req_.AnswerSlot().Json()["silent"].SetBool(true);
        return true;
    }

    bool IsCancellationSupported() const override {
        return true;
    }

    void SuggestForListing() override {
        // Empty body.
    }

    void PostProcessList(const TRemindersResult& reminders) override {
        if (!Ctx_.ClientFeatures().SupportsDiv2Cards() || reminders.List.ArraySize() < 1) {
            return;
        }

        TDiv2BlockBuilder div2block{"reminders_card", reminders.List, true};
        div2block.UseTemplate("reminders_card")
                 .UseTemplate("reminder")
                 .UseTemplate("reminders_header")
                 .SetTextFromNlgPhrase("render_result");
        Ctx_.AddDiv2CardBlock(std::move(div2block));
    }
};

void FillOnShootFrameIn(const TRequest& req, const NSc::TValue& payload, NSc::TValue& onShootFrame) {
    // Fill semantic frame in.
    auto& frame = onShootFrame["typed_semantic_frame"]["reminders_on_shoot_semantic_frame"];
    frame["id"]["string_value"] = payload["id"];
    frame["text"]["string_value"] = payload["text"];
    frame["epoch"]["epoch_value"] = payload["epoch"];
    frame["timezone"]["string_value"] = req.UserTZ.name();
    // Fill analyctics in.
    auto& analytics = onShootFrame["analytics"];
    analytics["product_scenario"].SetString("reminders");
    analytics["origin"].SetString("Scenario");
    analytics["origin_info"].SetString("");
    analytics["purpose"].SetString("on_shoot");
}

} // namespace

bool DeviceLocalRemindersCheckIfEnabled(TContext& ctx) {
    return ctx.ClientFeatures().SupportsDeviceLocalReminders();
}

void DeviceLocalRemindersCreateHandler(TRequest& req, TContext& ctx) {
    TakeDayPartIntoAccount(ctx);

    const auto text = req.ObtainRemindText();
    if (!text.Defined()) {
        return;
    }

    const auto shootTime = ProcessShootTimeValue(ctx, req);
    if (!shootTime.Defined()) {
        return;
    }

    const TString id = CreateGuidAsString();

    if (EPermissionType::Forbidden 
        == ctx.GetPermissionInfo(EClientPermission::ScheduleExactAlarm))
    {
        NSc::TValue directive;
        directive["name"].SetString("request_permissions");
        directive["permissions"].Push() = static_cast<int>(NAlice::TPermissions::ScheduleExactAlarm);

        NAlice::NRemindersApi::TReminderProto reminder;
        reminder.SetId(id);
        reminder.SetText(*text);
        reminder.SetShootAt(NDatetime::Convert(*shootTime, req.UserTZ).Seconds());
        reminder.SetTimeZone(req.UserTZ.name());

        directive["on_success"] = SeriazlizeToCallback("reminders_premission_on_success_callback", reminder);
        directive["on_fail"] = SeriazlizeToCallback("reminders_premission_on_fail_callback", reminder);

        ctx.AddCommand<TRequestPermissionsDirective>("request_permissions", std::move(directive));
        ctx.AddAttention("request_reminders_permission");
        ctx.AddStopListeningBlock();
        return;
    }

    {
        NSc::TValue directive;
        directive["id"].SetString(id);
        directive["text"].SetString(*text);
        directive["epoch"].SetString(ToString(NDatetime::Convert(*shootTime, req.UserTZ).Seconds()));
        directive["timezone"].SetString(req.UserTZ.name());

        NSc::TValue aux;
        aux["action"].SetString() = "id";
        aux["reminders"].Push() = directive.Clone();

        CREATION_SUCCESS_CALLBACK.PutInto(directive, aux);
        CREATION_FAIL_CALLBACK.PutInto(directive, aux);
        FillOnShootFrameIn(req, directive, directive["on_shoot_frame"]);

        ctx.AddCommand<TRemindersSetDirective>("reminders_set_directive", std::move(directive));
    }

    // This type is for vins to prevent say something and create nlg answer.
    req.AnswerSlot().Json()["silent"].SetBool(true);

    ctx.AddStopListeningBlock();
}

void DeviceLocalRemindersListHandler(TRequest& req, TContext& ctx) {
    TRemindersBackendLocal{req, ctx}.ProcessList();
}

void DeviceLocalRemindersCancelHandler(TRequest& req, TContext& ctx) {
    TRemindersBackendLocal{req, ctx}.ProcessCancel();
}

} // namespace NBASS::NReminders

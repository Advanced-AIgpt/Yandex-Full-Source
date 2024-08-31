#include "order_cancel_handler.h"

#include <alice/bass/forms/crmbot/context.h>
#include <alice/bass/forms/crmbot/forms.h>

#include <util/generic/serialized_enum.h>
#include <util/generic/vector.h>

namespace NBASS::NCrmbot {

void TOrderCancelHandler::Register(THandlersMap* handlers)
{
    handlers->emplace(
        ToString(EForm::ScenarioEntry),
        []() { return MakeHolder<TOrderCancelHandler>(); }
    );
    handlers->emplace(
        ToString(EForm::Data),
        []() { return MakeHolder<TOrderCancelHandler>(); }
    );
}

void TOrderCancelHandler::ChangeFormToScenarioEntry(TCrmbotContext& ctx) const
{
    if (FromString<EForm>(ctx.FormName()) != EForm::ScenarioEntry) {
        ctx.SetResponseFormAndCopySlots(ToString(EForm::ScenarioEntry), {
            TStringBuf("order_id_neuro"), TStringBuf("phone_number_neuro"), TStringBuf("email_neuro"),
            TStringBuf("order_id"), TStringBuf("phone_number"), TStringBuf("email")
        });
    }
}

void TOrderCancelHandler::HandleDelivered(TCrmbotContext& ctx, const TCheckouterOrder& order) const
{
    NSc::TValue data;
    data["orderId"] = order->Id();
    data["orderStatus"] = ToString(order.Status());
    ctx.AddTextCardBlock("order_delivered", data);
    ctx.AddSuggest("not_delivered");
}

void TOrderCancelHandler::HandleCancelled(TCrmbotContext& ctx, const TCheckouterOrder& order) const
{
    NSc::TValue data;
    data["orderId"] = order->Id();
    data["orderStatus"] = ToString(order.Status());
    ctx.AddTextCardBlock("order_cancelled", data);
}

void TOrderCancelHandler::MakeSingleOrderResponse(TCrmbotContext& ctx, const TCheckouterOrder& order) const
{
    switch (order.Status()) {
        case EOrderStatus::UNKNOWN:
        case EOrderStatus::PLACING:
        case EOrderStatus::RESERVED:
        {
            ctx.SetStringSlot("scenario_status", ToString(EScenarioStatus::RESOLVED_IN_ERROR));
            NSc::TValue data;
            data["orderId"] = order->Id();
            data["orderStatus"] = ToString(order.Status());
            ctx.AddTextCardBlock("order_not_found", data);
        }
        case EOrderStatus::DELIVERED:
        {
            ctx.SetStringSlot("scenario_status", ToString(EScenarioStatus::RESOLVED_IN_ERROR));
            HandleDelivered(ctx, order);
        }
        case EOrderStatus::CANCELLED:
        {
            ctx.SetStringSlot("scenario_status", ToString(EScenarioStatus::RESOLVED_IN_ERROR));
            HandleCancelled(ctx, order);
        } break;
        case EOrderStatus::PENDING:
        case EOrderStatus::UNPAID:
        case EOrderStatus::PICKUP:
        case EOrderStatus::PROCESSING:
        case EOrderStatus::DELIVERY:
        {
            if (order.HasCheckpoints({EDSCheckpointStatus::DELIVERY_TRANSMITTED_TO_RECIPIENT, EDSCheckpointStatus::DELIVERY_DELIVERED})) {
                HandleDelivered(ctx, order);
            } else if (order->HasCancellationRequest()) {
                HandleCancelled(ctx, order);
            } else {
                ctx.SetStringSlot("scenario_status", ToString(EScenarioStatus::ASKED_FOR_REASON));
                ctx.AddTextCardBlock("ask_reason");
                for (auto reason : GetEnumAllValues<EOrderCancellationReason>()) {
                    if (reason != EOrderCancellationReason::CUSTOM) { // remove this after MARKETCHECKOUT-14288
                        ctx.AddSuggest(TStringBuf("reason_") + ToString(reason));
                    }
                }
            }
        } break;
    }
}

void TOrderCancelReasonHandler::Register(THandlersMap* handlers)
{
    handlers->emplace(
        ToString(EForm::ReasonBoughtCheaper),
        []() { return MakeHolder<TOrderCancelReasonHandler>(); }
    );
    handlers->emplace(
        ToString(EForm::ReasonChangedMind),
        []() { return MakeHolder<TOrderCancelReasonHandler>(); }
    );
    handlers->emplace(
        ToString(EForm::ReasonOtherOrder),
        []() { return MakeHolder<TOrderCancelReasonHandler>(); }
    );
    handlers->emplace(
        ToString(EForm::ReasonCustom),
        []() { return MakeHolder<TOrderCancelReasonHandler>(); }
    );
    handlers->emplace(
        ToString(EForm::ReasonRefusedDelivery),
        []() { return MakeHolder<TOrderCancelReasonHandler>(); }
    );
}

TResultValue TOrderCancelReasonHandler::Do(TRequestHandler& r)
{
    TCrmbotContext ctx(r.Ctx());
    switch (FromString<EForm>(ctx.FormName())) {
        case EForm::ReasonBoughtCheaper:
            ctx.SetStringSlot("reason", ToString(EOrderCancellationReason::USER_BOUGHT_CHEAPER));
            SetReasonSelectedAndRespond(ctx);
            break;
        case EForm::ReasonChangedMind:
            ctx.SetStringSlot("reason", ToString(EOrderCancellationReason::USER_CHANGED_MIND));
            SetReasonSelectedAndRespond(ctx);
            break;
        case EForm::ReasonOtherOrder:
            ctx.SetStringSlot("reason", ToString(EOrderCancellationReason::USER_PLACED_OTHER_ORDER));
            SetReasonSelectedAndRespond(ctx);
            break;
        case EForm::ReasonRefusedDelivery:
            ctx.SetStringSlot("reason", ToString(EOrderCancellationReason::USER_REFUSED_DELIVERY));
            SetReasonSelectedAndRespond(ctx);
            break;
        case EForm::ReasonCustom:
            ctx.SetStringSlot("reason", ToString(EOrderCancellationReason::CUSTOM));
            SetReasonSelectedAndRespond(ctx);
            break;
        case EForm::ScenarioEntry:
        case EForm::Data:
        case EForm::Finish:
            ythrow TErrorException("Unexpected form handled");
    }
    return TResultValue();
}

void TOrderCancelReasonHandler::SetReasonSelectedAndRespond(TCrmbotContext& ctx) const
{
    ctx.SetStringSlot("scenario_status", ToString(EScenarioStatus::REASON_SELECTED));
    NSc::TValue data;
    data["orderId"] = ctx.GetStringSlot("order_id");
    ctx.AddTextCardBlock("thanks_for_feedback", data);
    ctx.AddTextCardBlock("ask_confirmation", data);
    ctx.AddSuggest("yes_cancel");
    ctx.AddSuggest("no_cancel");
}

void TOrderCancelFinishHandler::Register(THandlersMap* handlers)
{
    handlers->emplace(
        ToString(EForm::Finish),
        []() { return MakeHolder<TOrderCancelFinishHandler>(); }
    );
}

TResultValue TOrderCancelFinishHandler::Do(TRequestHandler& r)
{
    TCrmbotContext ctx(r.Ctx());
    TStringBuf confirmationValue = ctx.GetStringSlot("confirmation");

    if (confirmationValue == "yes") {
        TStringBuf orderId = ctx.GetStringSlot("order_id");
        auto reason = FromString<EOrderCancellationReason>(ctx.GetStringSlot("reason"));
        auto client = TCheckouterClient(ctx);
        auto result = client.CancelOrder(orderId, reason,
            TStringBuf("Заказ отменен по просьбе пользователя через чат."));
        if (result.Empty() || !result->Scheme()->HasCancellationRequest()) {
            ctx.SetStringSlot("scenario_status", ToString(EScenarioStatus::RESOLVED_IN_ERROR));
            ctx.AddTextCardBlock("cancel_failed");
        } else {
            NSc::TValue data;
            data["orderId"] = result->Scheme()->Id();
            data["status"] = result->Scheme()->Status();
            data["paymentMethod"] = result->Scheme()->PaymentMethod();
            data["paidWithThankyou"] = result->IsPaidWithThankyou();
            ctx.SetStringSlot("scenario_status", ToString(EScenarioStatus::CANCELLED));
            ctx.AddTextCardBlock("cancel_successful", data);
            AddFeedbackAddon(ctx);
        }
    } else if (confirmationValue == "no") {
        ctx.SetStringSlot("scenario_status", ToString(EScenarioStatus::CANCEL_ABORTED));
        ctx.AddTextCardBlock("ok_wont_cancel");
        AddFeedbackAddon(ctx);
    } else {
        ctx.AddTextCardBlock("ask_yes_no");
        ctx.AddSuggest("yes_cancel");
        ctx.AddSuggest("no_cancel");
    }

    return TResultValue();
}

}

#include "order_cancel_we_did_handler.h"

#include <alice/bass/forms/crmbot/context.h>

namespace NBASS::NCrmbot {

void TOrderCancelWeDidHandler::Register(THandlersMap* handlers)
{
    handlers->emplace(
        ToString(EForm::ScenarioEntry),
        []() { return MakeHolder<TOrderCancelWeDidHandler>(); }
    );
    handlers->emplace(
        ToString(EForm::Data),
        []() { return MakeHolder<TOrderCancelWeDidHandler>(); }
    );
}

void TOrderCancelWeDidHandler::ChangeFormToScenarioEntry(TCrmbotContext& ctx) const
{
    if (FromString<EForm>(ctx.FormName()) != EForm::ScenarioEntry) {
        ctx.SetResponseFormAndCopySlots(ToString(EForm::ScenarioEntry), {
            TStringBuf("order_id_neuro"), TStringBuf("phone_number_neuro"), TStringBuf("email_neuro"),
            TStringBuf("order_id"), TStringBuf("phone_number"), TStringBuf("email")
        });
    }
}

void TOrderCancelWeDidHandler::MakeSingleOrderResponse(TCrmbotContext& ctx, const TCheckouterOrder& order) const
{
    // First check if the order is indeed cancelled or has cancellation request
    EOrderSubstatus substatus;
    if (order.Status() == EOrderStatus::CANCELLED) {
        substatus = order.Substatus();
    } else if (order->HasCancellationRequest()) {
        substatus = FromString<EOrderSubstatus>(order->CancellationRequest()->Substatus());
    } else {
        NSc::TValue data;
        data["orderId"] = order->Id();
        data["orderStatus"] = ToString(order.Status());
        ctx.AddTextCardBlock("order_not_cancelled", data);
        ctx.AddSuggest("order_status");
        AddFeedbackAddon(ctx);
        return;
    }

    switch (substatus) {
        case EOrderSubstatus::ASYNC_PROCESSING:
        case EOrderSubstatus::DELIVERY_SERVICE_FAILED:
        case EOrderSubstatus::PENDING_EXPIRED:
        case EOrderSubstatus::PROCESSING_EXPIRED:
        case EOrderSubstatus::SERVICE_FAULT:
        case EOrderSubstatus::SHOP_FAILED:
        case EOrderSubstatus::SHOP_PENDING_CANCELLED:
        case EOrderSubstatus::USER_FRAUD:
        case EOrderSubstatus::WAREHOUSE_FAILED_TO_SHIP:
        // The following substatuses were not mentioned in CRMBOT-283
        case EOrderSubstatus::RESERVATION_EXPIRED:
        case EOrderSubstatus::USER_UNREACHABLE:
        case EOrderSubstatus::REPLACING_ORDER:
        case EOrderSubstatus::RESERVATION_FAILED:
        case EOrderSubstatus::WRONG_ITEM:
        case EOrderSubstatus::LATE_CONTACT:
        case EOrderSubstatus::DELIVERY_SERIVCE_UNDELIVERED:
        case EOrderSubstatus::PREORDER:
        case EOrderSubstatus::AWAIT_CONFIRMATION:
        case EOrderSubstatus::STARTED:
        case EOrderSubstatus::PACKAGING:
        case EOrderSubstatus::READY_TO_SHIP:
        case EOrderSubstatus::SHIPPED:
        case EOrderSubstatus::USER_REFUSED_TO_PROVIDE_PERSONAL_DATA:
        case EOrderSubstatus::WAITING_USER_INPUT:
        case EOrderSubstatus::WAITING_BANK_DECISION:
        case EOrderSubstatus::AWAIT_DELIVERY_DATES_CONFIRMATION:
        case EOrderSubstatus::DELIVERY_SERVICE_RECEIVED:
        case EOrderSubstatus::USER_RECEIVED:
        case EOrderSubstatus::WAITING_FOR_STOCKS:
        case EOrderSubstatus::UNKNOWN:
            ctx.SetStringSlot("scenario_status", ToString(EScenarioStatus::REDIRECTED_TO_OPERATOR));
            ctx.AddTextCardBlock("call_operator");
            if (ctx.IsWebim1()) {
                ctx.AddSuggest("call_operator");
            }
            break;
        case EOrderSubstatus::BANK_REJECT_CREDIT_OFFER:
        case EOrderSubstatus::BROKEN_ITEM:
        case EOrderSubstatus::CREDIT_OFFER_FAILED:
        case EOrderSubstatus::CUSTOM:
        case EOrderSubstatus::CUSTOMER_REJECT_CREDIT_OFFER:
        case EOrderSubstatus::DELIVERY_PROBLEMS:
        case EOrderSubstatus::MISSING_ITEM:
        case EOrderSubstatus::PENDING_CANCELLED:
        case EOrderSubstatus::PICKUP_EXPIRED:
        case EOrderSubstatus::USER_BOUGHT_CHEAPER:
        case EOrderSubstatus::USER_CHANGED_MIND:
        case EOrderSubstatus::USER_NOT_PAID:
        case EOrderSubstatus::USER_PLACED_OTHER_ORDER:
        case EOrderSubstatus::USER_REFUSED_DELIVERY:
        case EOrderSubstatus::USER_REFUSED_PRODUCT:
        case EOrderSubstatus::USER_REFUSED_QUALITY:
            ctx.SetStringSlot("scenario_status", ToString(EScenarioStatus::REASON_STATED));
            ctx.AddTextCardBlock(TStringBuf("reason_")+ToString(substatus));
            AddFeedbackAddon(ctx);
            break;
    }
}

void TOrderCancelWeDidContinuationHandler::Register(THandlersMap* handlers)
{
    handlers->emplace(
        ToString(EForm::Continuation),
        []() { return MakeHolder<TOrderCancelWeDidContinuationHandler>(); }
    );
}

TResultValue TOrderCancelWeDidContinuationHandler::Do(TRequestHandler& r)
{
    TCrmbotContext ctx(r.Ctx());
    TStringBuf confirmationValue = ctx.GetStringSlot("confirmation");

    auto scenarioStatus = FromString<EScenarioStatus>(ctx.GetStringSlot("scenario_status"));

    switch (scenarioStatus) {
        case EScenarioStatus::REASON_STATED:
        {
            if (confirmationValue == "yes") {
                ctx.SetResponseFormAndCopySlots("crm_bot.scenarios.feedback_positive", {TStringBuf("confirmation")});
            } else if (confirmationValue == "no") {
                ctx.SetResponseFormAndCopySlots("crm_bot.scenarios.feedback_negative", {TStringBuf("confirmation")});
            } else {
                // we asked for feedback and got activation here without recognising "yes" or "no"
                // something is clearly broken in classifier
                ctx.SetResponseForm("crm_bot.scenarios.operator_redirect");
            }
            break;
        }
        case EScenarioStatus::REDIRECTED_TO_OPERATOR:
        {
            if (confirmationValue == "yes") {
                ctx.SetResponseForm("crm_bot.scenarios.operator_redirect");
            } else if (confirmationValue == "no") {
                ctx.AddTextCardBlock("cancel_operator_redirect");
                ctx.Ctx().DeleteSlot("scenario_status");
            } else {
                // we asked for redirect and got activation here without recognising "yes" or "no"
                // something is clearly broken in classifier
                ctx.SetResponseForm("crm_bot.scenarios.operator_redirect");
            }
            break;
        }
    }

    return TResultValue();
}

}

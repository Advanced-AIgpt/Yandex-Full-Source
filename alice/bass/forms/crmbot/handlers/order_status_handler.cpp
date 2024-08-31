#include "order_status_handler.h"

#include <alice/bass/forms/crmbot/clients/checkouter.h>
#include <alice/bass/forms/crmbot/context.h>
#include <alice/bass/forms/crmbot/order_status.h>
#include <alice/bass/forms/crmbot/personal_data_helper.h>

#include <util/generic/vector.h>
#include <util/string/join.h>

#include <alice/bass/forms/crmbot/forms.h>
#include <regex>

namespace NBASS::NCrmbot {

void TOrderStatusHandler::Register(THandlersMap* handlers)
{
    handlers->emplace(
        ToString(EForm::ScenarioEntry),
        []() { return MakeHolder<TOrderStatusHandler>(); }
    );
    handlers->emplace(
        ToString(EForm::Data),
        []() { return MakeHolder<TOrderStatusHandler>(); }
    );
}

void TOrderStatusHandler::ChangeFormToScenarioEntry(TCrmbotContext& ctx) const
{
    if (FromString<EForm>(ctx.FormName()) != EForm::ScenarioEntry) {
        ctx.SetResponseFormAndCopySlots(ToString(EForm::ScenarioEntry), {
            TStringBuf("order_id_neuro"), TStringBuf("phone_number_neuro"), TStringBuf("email_neuro"),
            TStringBuf("order_id"), TStringBuf("phone_number"), TStringBuf("email")
        });
    }
}

bool TOrderStatusHandler::AreDatesChanged(
    TCheckouterDeliveryDatesRef before, TCheckouterDeliveryDatesRef after) const
{
    bool changed =
        after->FromDate().Get() != before->FromDate().Get() || after->ToDate().Get() != before->ToDate().Get();
    if (!changed && after->HasToTime() && after->HasFromTime() && before->HasToTime() && before->HasFromTime()) {
        changed =
            after->FromTime().Get() != before->FromTime().Get() || after->ToTime().Get() != before->ToTime().Get();
    }
    return changed;
}

TOrderStatusHandler::TDeliveryDeadline
TOrderStatusHandler::GetUserDeliveryDeadline(
    const TVector<TCheckouterOrderChange>& changes) const
{
    // This method's logic depends on events being sorted by date in descending order
    TDeliveryDeadline deadline;
    bool isLastChange = true;
    for (const auto& event: changes) {
        if (AreDatesChanged(event->OrderBefore()->Delivery()->Dates(), event->OrderAfter()->Delivery()->Dates())) {
            deadline.WasChanged = true;
            if (event->HasReason()) {
                auto reason = FromString<EDeliveryChangeReason>(event->Reason());
                bool byUser = (reason == EDeliveryChangeReason::USER_MOVED_DELIVERY_DATES);
                if (isLastChange) { deadline.WasChangedByUser = byUser; }
                if (byUser) {
                    deadline.Deadline = event->OrderAfter()->Delivery()->Dates()->GetRawValue();
                    return deadline;
                }
            }
            isLastChange = false;
        }
    }
    deadline.Deadline = changes.back()->OrderBefore()->Delivery()->Dates()->GetRawValue();
    return deadline;
}

void TOrderStatusHandler::HandleDelivered(const NSc::TValue& contextData, TCrmbotContext& ctx) const
{
    ctx.SetStringSlot("scenario_status", ToString(EScenarioStatus::DELIVERED));
    ctx.AddTextCardBlock("order_delivered", contextData);
    ctx.AddSuggest("not_delivered");
    AddFeedbackAddon(ctx);
}

void TOrderStatusHandler::HandleComplexStatuses(
    const TCheckouterOrder& order, TCrmbotContext& ctx) const
{
    // get delivery timezone
    ui32 regionId = order->Delivery()->RegionId();
    const auto& geobase = ctx.Ctx().GlobalCtx().GeobaseLookup();
    NGeobase::TRegion region = geobase.GetRegionById(regionId);
    NDatetime::TTimeZone deliveryTz = NDatetime::GetTimeZone(region.GetTimezoneName());
    // get get date and time in the delivery timezone
    TInstant now = ctx.Ctx().Now();
    NDatetime::TSimpleTM today = NDatetime::ToCivilTime(now, deliveryTz);
    TString todayDateStr = today.ToString("%d-%m-%Y");
    TString todayTimeStr = today.ToString("%H:%M");
    TInstant todayDate = TCheckouterOrder::ParseDatetime(todayDateStr, "%d-%m-%Y"); // do it this way for consistency
    // get original and current delivery deadlines

    TCheckouterClient client(ctx);
    const auto& orderChanges =
        client.GetOrderChanges(ToString(order->Id()), {EOrderChangeType::ORDER_DELIVERY_UPDATED});
    const NSc::TValue currentDeadline = *(order->Delivery()->Dates()->GetRawValue());
    auto userDeliveryDeadline = GetUserDeliveryDeadline(orderChanges);
    TInstant deliveryDeadline = TCheckouterOrder::ParseDatetime(currentDeadline["toDate"], "%d-%m-%Y");
    TInstant orderCreationDate = TCheckouterOrder::ParseDatetime(order->CreationDate(), "%d-%m-%Y %H:%m-%s");

    NSc::TValue contextData{};
    contextData["orderId"] = order->Id();
    contextData["was_rescheduled"] = userDeliveryDeadline.WasChanged;
    contextData["was_rescheduled_by_user"] = userDeliveryDeadline.WasChangedByUser;
    contextData["delivery_type"] = ToString(order.DeliveryType());
    contextData["delivery_dates"] = currentDeadline;
    contextData["was_logged_in"] = order->Buyer()->HasUid() && order->Buyer()->Uid().Get() != 0;
    if (order->Delivery()->HasOutlet())
        contextData["outlet"] = *(order->Delivery()->Outlet()->GetRawValue());
    if (order->Delivery()->HasBuyerAddress())
        contextData["buyerAddress"] = *(order->Delivery()->BuyerAddress()->GetRawValue());

    enum class ERelativeDeliveryDate { PAST, TODAY, FUTURE };
    ERelativeDeliveryDate moment;
    if (todayDate == deliveryDeadline) {
        if (order->Delivery()->Dates()->HasFromTime() && order->Delivery()->Dates()->FromTime() > todayTimeStr) {
            moment = ERelativeDeliveryDate::FUTURE;
        } else { moment = ERelativeDeliveryDate::TODAY; }
    }
    else if (todayDate < deliveryDeadline) { moment = ERelativeDeliveryDate::FUTURE; }
    else { moment = ERelativeDeliveryDate::PAST; }

    switch (moment) {
        case ERelativeDeliveryDate::TODAY: {
            if (order.HasCheckpoints({EDSCheckpointStatus::DELIVERY_TRANSPORTATION_RECIPIENT})) {
                // its ok
                if (order->Delivery()->Dates()->HasToTime() && order->Delivery()->Dates()->ToTime() > todayTimeStr) {
                    ctx.SetStringSlot("scenario_status", ToString(EScenarioStatus::DELIVERY_TODAY_FINE));
                    ctx.AddTextCardBlock("delivery_today_fine", contextData);
                    if (order.DeliveryType() == EDeliveryType::PICKUP) {
                        ctx.AddSuggest("where_is_pickup_location");
                    }
                    AddFeedbackAddon(ctx);
                } else {
                    TStringBuf deliveryServiceContact = order.DeliveryServiceContact();
                    contextData["deliveryServiceContact"] = deliveryServiceContact;
                    ctx.SetStringSlot("scenario_status", ToString(EScenarioStatus::DELIVERY_TODAY_COURIER_IN_TRAFFIC));
                    ctx.AddTextCardBlock("delivery_today_courier_in_traffic", contextData);
                    ctx.AddTextCardBlock("wanna_wait_question");
                    if (order.DeliveryType() == EDeliveryType::PICKUP) {
                        ctx.AddSuggest("where_is_pickup_location");
                    }
                    if (!deliveryServiceContact.Empty()) {
                        ctx.AddSuggest("dont_wanna_call");
                    } else {
                        ctx.AddSuggest("yes_wait");
                        ctx.AddSuggest("no_wait");
                    }
                }
            } else {
                if (order.Status() == EOrderStatus::PROCESSING) {
                    if (order->Fulfilment() || now - orderCreationDate < TDuration::Days(2)) {
                        ctx.SetStringSlot("scenario_status", ToString(EScenarioStatus::DELIVERY_TODAY_LOST));
                        ctx.AddTextCardBlock("delivery_today_order_lost");
                        ctx.AddTextCardBlock("wanna_wait_to_find");
                        ctx.AddSuggest("yes_contact");
                        ctx.AddSuggest("no_contact");
                    } else {
                        // we'll have to cancel this order in future
                        ctx.SetStringSlot("scenario_status",
                                          ToString(EScenarioStatus::DELIVERY_TODAY_LOST_COMPLETELY));
                        ctx.AddTextCardBlock("call_operator");
                        if (ctx.IsWebim1()) {
                            ctx.AddSuggest("call_operator");
                        }
                    }
                } else {
                    // should be order.Status() == EOrderStatus::DELIVERY
                    // we need a human to call delivery service and sort everything out
                    ctx.SetStringSlot("scenario_status", ToString(EScenarioStatus::DELIVERY_TODAY_LOST_CALL));
                    ctx.AddTextCardBlock("call_operator");
                    if (ctx.IsWebim1()) {
                        ctx.AddSuggest("call_operator");
                    }
                }
            }
        } break;
        case ERelativeDeliveryDate::FUTURE: {
            bool shipmentDelayed = false;
            TInstant maxShipmentDate = order.MaxShipmentDateTime(shipmentDelayed); // in MSK timezone
            if ((maxShipmentDate + TDuration::Days(1) > now && !shipmentDelayed) ||
                order.HasCheckpoints({EDSCheckpointStatus::SORTING_CENTER_TRANSMITTED})) {
                // All good
                ctx.AddTextCardBlock("delivery_future", contextData);
                if (order.DeliveryType() == EDeliveryType::PICKUP) {
                    ctx.AddSuggest("where_is_pickup_location");
                }
                if (userDeliveryDeadline.WasChanged && !userDeliveryDeadline.WasChangedByUser) {
                    ctx.SetStringSlot("scenario_status", ToString(EScenarioStatus::DELIVERY_FUTURE_RESCHEDULED));
                    ctx.AddTextCardBlock("wanna_wait_reschedule");
                    ctx.AddSuggest("yes_rescheduled");
                    ctx.AddSuggest("yes_change_address");
                    ctx.AddSuggest("no_rescheduled");
                } else {
                    ctx.SetStringSlot("scenario_status", ToString(EScenarioStatus::DELIVERY_FUTURE_FINE));
                    AddFeedbackAddon(ctx);
                }
            } else {
                if (order->Fulfilment() || now - orderCreationDate < TDuration::Days(2)) {
                    ctx.SetStringSlot("scenario_status", ToString(EScenarioStatus::DELIVERY_FUTURE_LOST));
                    ctx.AddTextCardBlock("delivery_future_lost");
                    ctx.AddTextCardBlock("wanna_wait_to_find");
                    ctx.AddSuggest("yes_contact");
                    ctx.AddSuggest("no_contact");
                } else {
                    // we'll have to cancel this order in future
                    ctx.SetStringSlot("scenario_status",
                                      ToString(EScenarioStatus::DELIVERY_FUTURE_LOST_COMPLETELY));
                    ctx.AddTextCardBlock("call_operator");
                    if (ctx.IsWebim1()) {
                        ctx.AddSuggest("call_operator");
                    }
                }
            }
        } break;
        case ERelativeDeliveryDate::PAST: {
            if (order.HasCheckpoints({EDSCheckpointStatus::DELIVERY_TRANSPORTATION_RECIPIENT})) {
                // it's messed up but the courier is already on his way
                // we can't trust delivery intervals so we assume we're late
                TStringBuf deliveryServiceContact = order.DeliveryServiceContact();
                contextData["deliveryServiceContact"] = deliveryServiceContact;
                ctx.SetStringSlot("scenario_status", ToString(EScenarioStatus::DELIVERY_MESSED_UP_BUT_STILL_HAPPENING));
                ctx.AddTextCardBlock("delivery_past", contextData);
                if (order.DeliveryType() == EDeliveryType::PICKUP) {
                    ctx.AddSuggest("where_is_pickup_location");
                }
                AddFeedbackAddon(ctx);
            } else {
                ctx.SetStringSlot("scenario_status", ToString(EScenarioStatus::DELIVERY_MESSED_UP));
                ctx.AddTextCardBlock("call_operator");
                if (ctx.IsWebim1()) {
                    ctx.AddSuggest("call_operator");
                }
            }
        } break;
    }
}

void TOrderStatusHandler::MakeSingleOrderResponse(TCrmbotContext& ctx, const TCheckouterOrder& order) const
{
    // First Check if the order has been already delivered or cancelled or anything
    NSc::TValue contextData{};
    contextData["orderId"] = order->Id();
    if (order->HasCancellationRequest() && order.Status() != EOrderStatus::CANCELLED) {
        ctx.SetStringSlot("scenario_status", ToString(EScenarioStatus::HAS_CANCELLATION_REQUEST));
        ctx.AddTextCardBlock("order_has_cancellation_request", contextData);
        ctx.AddSuggest("not_cancelled");
        AddFeedbackAddon(ctx);
    } else {
        switch (order.Status()) {
            case EOrderStatus::UNKNOWN:
            case EOrderStatus::PLACING:
            case EOrderStatus::RESERVED:
                // The user should not know about this order at all
                ctx.SetStringSlot("scenario_status", ToString(EScenarioStatus::NOT_FOUND));
                ctx.AddTextCardBlock("order_not_found", contextData);
                break;
            case EOrderStatus::UNPAID:
                ctx.SetStringSlot("scenario_status", ToString(EScenarioStatus::UNPAID));
                ctx.AddTextCardBlock("order_unpaid", contextData);
                AddFeedbackAddon(ctx);
                break;
            case EOrderStatus::PENDING:
                ctx.SetStringSlot("scenario_status", ToString(EScenarioStatus::PENDING));
                ctx.AddTextCardBlock("order_pending", contextData);
                AddFeedbackAddon(ctx);
                break;
            case EOrderStatus::DELIVERED:
                HandleDelivered(contextData, ctx);
                break;
            case EOrderStatus::CANCELLED:
                ctx.SetStringSlot("scenario_status", ToString(EScenarioStatus::CANCELLED));
                ctx.AddTextCardBlock("order_cancelled", contextData);
                ctx.AddSuggest("not_cancelled");
                AddFeedbackAddon(ctx);
                break;
            case EOrderStatus::PICKUP:
                if (order.HasCheckpoints({EDSCheckpointStatus::DELIVERY_TRANSMITTED_TO_RECIPIENT,
                                          EDSCheckpointStatus::DELIVERY_DELIVERED})) {
                    HandleDelivered(contextData, ctx);
                } else {

                    ctx.SetStringSlot("scenario_status", ToString(EScenarioStatus::PICKUP));
                    ctx.AddTextCardBlock("order_in_pickup", contextData);
                    ctx.AddSuggest("where_is_pickup_location");
                    AddFeedbackAddon(ctx);
                }
                break;
            case EOrderStatus::PROCESSING:
            case EOrderStatus::DELIVERY:
                if (order.HasCheckpoints({EDSCheckpointStatus::DELIVERY_TRANSMITTED_TO_RECIPIENT,
                                          EDSCheckpointStatus::DELIVERY_DELIVERED})) {
                    HandleDelivered(contextData, ctx);
                } else {
                    HandleComplexStatuses(order, ctx);
                }
                break;
        }
    }
}

void TOrderStatusContinuationHandler::Register(THandlersMap* handlers)
{
    handlers->emplace(
        ToString(EForm::ChangeAddress),
        []() { return MakeHolder<TOrderStatusContinuationHandler>(); }
    );
    handlers->emplace(
        ToString(EForm::Continuation),
        []() { return MakeHolder<TOrderStatusContinuationHandler>(); }
    );
}

TResultValue TOrderStatusContinuationHandler::Do(TRequestHandler& r)
{
    TCrmbotContext ctx(r.Ctx());

    TStringBuf confirmationValue = ctx.GetStringSlot("confirmation");

    if (FromString<EForm>(ctx.FormName()) == EForm::ChangeAddress) {
        ctx.SetStringSlot("scenario_status", ToString(EScenarioStatus::RESOLVED_IN_REDIRECT));
        ctx.AddTextCardBlock("operator_redirect_after_change_address");
    } else {
        auto status = FromString<EScenarioStatus>(ctx.GetStringSlot("scenario_status"));
        if (confirmationValue == "yes") {
            switch (status) {
                case EScenarioStatus::DELIVERY_FUTURE_RESCHEDULED:
                case EScenarioStatus::DELIVERY_TODAY_COURIER_IN_TRAFFIC:
                    ctx.SetStringSlot("scenario_status", ToString(EScenarioStatus::RESOLVED_IN_THANKS));
                    ctx.AddTextCardBlock("thank_you");
                    AddFeedbackAddon(ctx);
                    break;
                case EScenarioStatus::DELIVERY_FUTURE_LOST:
                case EScenarioStatus::DELIVERY_TODAY_LOST:
                    ctx.SetStringSlot("scenario_status", ToString(EScenarioStatus::RESOLVED_IN_THANKS));
                    ctx.AddTextCardBlock("thank_you_lost");
                    AddFeedbackAddon(ctx);
                    break;
                case EScenarioStatus::DELIVERY_TODAY_FINE:
                case EScenarioStatus::DELIVERY_FUTURE_FINE:
                case EScenarioStatus::RESOLVED_IN_THANKS:
                case EScenarioStatus::HAS_CANCELLATION_REQUEST:
                case EScenarioStatus::DELIVERY_MESSED_UP_BUT_STILL_HAPPENING:
                case EScenarioStatus::UNPAID:
                case EScenarioStatus::PENDING:
                case EScenarioStatus::DELIVERED:
                case EScenarioStatus::CANCELLED:
                case EScenarioStatus::PICKUP:
                    // erroneous activation
                    ctx.SetResponseFormAndCopySlots("crm_bot.scenarios.feedback_positive", {TStringBuf("confirmation")});
                    break;
                case EScenarioStatus::DELIVERY_TODAY_LOST_COMPLETELY:
                case EScenarioStatus::DELIVERY_TODAY_LOST_CALL:
                case EScenarioStatus::DELIVERY_FUTURE_LOST_COMPLETELY:
                case EScenarioStatus::DELIVERY_MESSED_UP:
                    if (ctx.IsWebim1()) {
                        // We asked to press a button for operator redirect however the button should be handled by
                        // the crm_bot.scenarios.operator_redirect scenario.
                        // If the used said something like "YES" redirect him in that case.
                        ctx.SetResponseForm("crm_bot.scenarios.operator_redirect");
                        break;
                    }
                case EScenarioStatus::RESOLVED_IN_REDIRECT:
                case EScenarioStatus::NOT_FOUND:
                    // we asked nothing
                    ctx.SetResponseForm("crm_bot.scenarios.garbage");
                    break;
            }
        } else if (confirmationValue == "no") {
            switch (status) {
                case EScenarioStatus::DELIVERY_FUTURE_RESCHEDULED:
                case EScenarioStatus::DELIVERY_FUTURE_LOST:
                case EScenarioStatus::DELIVERY_TODAY_LOST:
                case EScenarioStatus::DELIVERY_TODAY_COURIER_IN_TRAFFIC:
                    ctx.SetStringSlot("scenario_status", ToString(EScenarioStatus::RESOLVED_IN_REDIRECT));
                    ctx.AddTextCardBlock("operator_redirect_after_continuation");
                    break;
                case EScenarioStatus::DELIVERY_TODAY_FINE:
                case EScenarioStatus::DELIVERY_FUTURE_FINE:
                case EScenarioStatus::RESOLVED_IN_THANKS:
                case EScenarioStatus::HAS_CANCELLATION_REQUEST:
                case EScenarioStatus::DELIVERY_MESSED_UP_BUT_STILL_HAPPENING:
                case EScenarioStatus::UNPAID:
                case EScenarioStatus::PENDING:
                case EScenarioStatus::DELIVERED:
                case EScenarioStatus::CANCELLED:
                case EScenarioStatus::PICKUP:
                    // erroneous activation
                    ctx.SetResponseFormAndCopySlots("crm_bot.scenarios.feedback_negative", {TStringBuf("confirmation")});
                    break;
                case EScenarioStatus::DELIVERY_TODAY_LOST_COMPLETELY:
                case EScenarioStatus::DELIVERY_TODAY_LOST_CALL:
                case EScenarioStatus::DELIVERY_FUTURE_LOST_COMPLETELY:
                case EScenarioStatus::DELIVERY_MESSED_UP:
                    if (ctx.IsWebim1()) {
                        // We asked to press a button for operator redirect however the button should be handled by
                        // the crm_bot.scenarios.operator_redirect scenario.
                        // If the used said something like "NO" don't redirect, but clear the status slot
                        ctx.AddTextCardBlock("cancel_operator_redirect");
                        ctx.Ctx().DeleteSlot("scenario_status");
                        break;
                    }
                case EScenarioStatus::RESOLVED_IN_REDIRECT:
                case EScenarioStatus::NOT_FOUND:
                    // we asked nothing
                    ctx.SetResponseForm("crm_bot.scenarios.garbage");
                    break;
            }
        } else {
            switch (status) {
                case EScenarioStatus::DELIVERY_TODAY_COURIER_IN_TRAFFIC:
                    ctx.AddTextCardBlock("ask_yes_no");
                    ctx.AddSuggest("yes_wait");
                    ctx.AddSuggest("no_wait");
                    break;
                case EScenarioStatus::DELIVERY_FUTURE_RESCHEDULED:
                    ctx.AddSuggest("yes_rescheduled");
                    ctx.AddSuggest("yes_change_address");
                    ctx.AddSuggest("no_rescheduled");
                    break;
                case EScenarioStatus::DELIVERY_FUTURE_LOST:
                case EScenarioStatus::DELIVERY_TODAY_LOST:
                    ctx.AddTextCardBlock("ask_yes_no");
                    ctx.AddSuggest("yes_contact");
                    ctx.AddSuggest("no_contact");
                    break;
                case EScenarioStatus::DELIVERY_TODAY_FINE:
                case EScenarioStatus::DELIVERY_FUTURE_FINE:
                case EScenarioStatus::RESOLVED_IN_THANKS:
                case EScenarioStatus::HAS_CANCELLATION_REQUEST:
                case EScenarioStatus::DELIVERY_MESSED_UP_BUT_STILL_HAPPENING:
                case EScenarioStatus::UNPAID:
                case EScenarioStatus::PENDING:
                case EScenarioStatus::DELIVERED:
                case EScenarioStatus::CANCELLED:
                case EScenarioStatus::PICKUP:
                    // have feedback suggests but, since the tagger has already failed,
                    // we won't torture the user and assume garbage here
                case EScenarioStatus::DELIVERY_TODAY_LOST_COMPLETELY:
                case EScenarioStatus::DELIVERY_TODAY_LOST_CALL:
                case EScenarioStatus::DELIVERY_FUTURE_LOST_COMPLETELY:
                case EScenarioStatus::DELIVERY_MESSED_UP:
                case EScenarioStatus::RESOLVED_IN_REDIRECT:
                case EScenarioStatus::NOT_FOUND:
                    // we asked nothing
                    ctx.SetResponseForm("crm_bot.scenarios.garbage");
                    break;
            }
        }
    }

    return TResultValue();
}

void TOrderStatusPickupWhereHandler::Register(THandlersMap* handlers)
{
    handlers->emplace(
        ToString(EForm::PickupLocationWhere),
        []() { return MakeHolder<TOrderStatusPickupWhereHandler>(); }
    );
}

void TOrderStatusPickupWhereHandler::ChangeFormToScenarioEntry(TCrmbotContext&) const
{  // nothing to do here - we do not activate on multiple forms
}

void TOrderStatusPickupWhereHandler::MakeSingleOrderResponse(TCrmbotContext& ctx, const TCheckouterOrder& order) const
{
    NSc::TValue data{};
    data["orderId"] = ctx.GetStringSlot("order_id");
    if (order.DeliveryType() != EDeliveryType::PICKUP) {
        // erroneous activation: user asked for possible locations after asking about status
        // hard to disable in transition model
        ctx.SetResponseForm("crm_bot.scenarios.pickup_location_find");
    }
    if (order->Delivery()->HasOutlet()) {
        data["outlet"] = *(order->Delivery()->Outlet()->GetRawValue());
        ctx.AddTextCardBlock("pickup_location", data);
        AddFeedbackAddon(ctx);
    } else {
        ctx.AddTextCardBlock("no_pickup_location", data); // should not happen
        ctx.AddSuggest("call_operator");
    }
}

}


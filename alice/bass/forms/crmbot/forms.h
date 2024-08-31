#pragma once

#include <util/generic/strbuf.h>

namespace NBASS::NCrmbot {

enum class ESearchForm {
    Search /* "crm_bot.scenarios.search" */,
    SearchWhere /* "crm_bot.scenarios.search__where" */
};
enum class ESearchIntent {
    PriceCheck /* "price_check" */,
    StockCheck /* "stock_check" */,
    OutOfStock /* "out_of_stock" */,
    NoIntent /* "" */
};

enum class EOrderStatusForm {
    ScenarioEntry /* "crm_bot.scenarios.order_status" */,
    Data /* "crm_bot.scenarios.order_status__data" */,
    Continuation /* "crm_bot.scenarios.order_status__continuation" */,
    ChangeAddress /* "crm_bot.scenarios.order_status__change_address" */,
    PickupLocationWhere /* "crm_bot.scenarios.order_status__where_is_pickup_location" */
};

enum class EOrderStatusScenarioStatus {
    HAS_CANCELLATION_REQUEST /* "HAS_CANCELLATION_REQUEST" */,
    NOT_FOUND /* "NOT_FOUND" */,
    UNPAID /* "UNPAID" */,
    PENDING /* "PENDING" */,
    DELIVERED /* "DELIVERED" */,
    CANCELLED /* "CANCELLED" */,
    PICKUP /* "PICKUP" */,
    DELIVERY_TODAY_FINE /* "DELIVERY_TODAY_FINE" */,
    DELIVERY_TODAY_COURIER_IN_TRAFFIC /* "DELIVERY_TODAY_COURIER_IN_TRAFFIC" */,
    DELIVERY_TODAY_LOST /* "DELIVERY_TODAY_LOST" */,
    DELIVERY_TODAY_LOST_COMPLETELY /* "DELIVERY_TODAY_LOST_COMPLETELY" */,
    DELIVERY_TODAY_LOST_CALL /* "DELIVERY_TODAY_LOST_CALL" */,
    DELIVERY_FUTURE_FINE /* "DELIVERY_FUTURE_FINE" */,
    DELIVERY_FUTURE_RESCHEDULED /* "DELIVERY_FUTURE_RESCHEDULED" */,
    DELIVERY_FUTURE_LOST /* "DELIVERY_FUTURE_LOST" */,
    DELIVERY_FUTURE_LOST_COMPLETELY /* "DELIVERY_FUTURE_LOST_COMPLETELY" */,
    DELIVERY_MESSED_UP /* "DELIVERY_MESSED_UP" */,
    DELIVERY_MESSED_UP_BUT_STILL_HAPPENING /* "DELIVERY_MESSED_UP_BUT_STILL_HAPPENING" */,
    RESOLVED_IN_THANKS /* "RESOLVED_IN_THANKS" */,
    RESOLVED_IN_REDIRECT /* "RESOLVED_IN_REDIRECT" */
};

enum class EOrderCancelForm {
    ScenarioEntry /* "crm_bot.scenarios.order_cancel_for_me" */,
    Data /* "crm_bot.scenarios.order_cancel_for_me__data" */,
    ReasonBoughtCheaper /* "crm_bot.scenarios.order_cancel_for_me__bought_cheaper" */,
    ReasonChangedMind /* "crm_bot.scenarios.order_cancel_for_me__changed_mind" */,
    ReasonOtherOrder /* "crm_bot.scenarios.order_cancel_for_me__placed_other_order" */,
    ReasonRefusedDelivery /* "crm_bot.scenarios.order_cancel_for_me__refused_delivery" */,
    ReasonCustom /* "crm_bot.scenarios.order_cancel_for_me__reason" */,
    Finish /* "crm_bot.scenarios.order_cancel_for_me_finish" */
};

enum class EOrderCancelScenarioStatus {
    ASKED_FOR_REASON,
    REASON_SELECTED,
    CANCELLED,
    CANCEL_ABORTED,
    RESOLVED_IN_ERROR
};

enum class EOrderCancelWeDidForm {
    ScenarioEntry /* "crm_bot.scenarios.order_cancel_we_did" */,
    Data /* "crm_bot.scenarios.order_cancel_we_did__data" */,
    Continuation /* "crm_bot.scenarios.order_cancel_we_did__continuation" */,
};

enum class EOrderCancelWeDidScenarioStatus {
    REASON_STATED,
    REDIRECTED_TO_OPERATOR
};

} // namespace NBASS::NCrmbot

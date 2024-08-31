#pragma once

#include "handler.h"
#include <util/generic/hash_set.h>

namespace NBASS::NPushNotification {
// taxi events
// https://wiki.yandex-team.ru/users/kravchusha/Alicecalltaxi/#pushi
constexpr TStringBuf ON_ASSIGNED = "on_assigned";
constexpr TStringBuf ON_ASSIGNED_EXACT = "on_assigned_exact";
constexpr TStringBuf ON_AUTOREORDER_TIMEOUT = "on_autoreorder_timeout";
constexpr TStringBuf ON_DEBT_ALLOWED = "on_debt_allowed";
constexpr TStringBuf ON_DRIVER_ARRIVING = "on_driver_arriving";
constexpr TStringBuf ON_FAILED = "on_failed";
constexpr TStringBuf ON_FAILED_PRICE = "on_failed_price";
constexpr TStringBuf ON_FAILED_PRICE_WITH_COUPON = "on_failed_price_with_coupon";
constexpr TStringBuf ON_MOVED_TO_CASH = "on_moved_to_cash";
constexpr TStringBuf ON_MOVED_TO_CASH_WITH_COUPON = "on_moved_to_cash_with_coupon";
constexpr TStringBuf ON_SEARCH_FAILED = "on_search_failed";
constexpr TStringBuf ON_WAITING = "on_waiting";

// local events
constexpr TStringBuf LOCAL_ADD_PHONE_IN_PASSPORT = "local_add_phone_in_passport";
constexpr TStringBuf LOCAL_CALL_TO_DRIVER = "local_call_to_driver";
constexpr TStringBuf LOCAL_CALL_TO_SUPPORT = "local_call_to_support";
constexpr TStringBuf LOCAL_SEND_LEGAL = "local_send_legal";
constexpr TStringBuf LOCAL_WHO_IS_TRANSPORTER = "local_who_is_transporter";

const THashSet<TStringBuf> EVENTS{ON_ASSIGNED_EXACT, ON_AUTOREORDER_TIMEOUT, ON_DEBT_ALLOWED, ON_DRIVER_ARRIVING,
                                  ON_FAILED, ON_FAILED_PRICE, ON_FAILED_PRICE_WITH_COUPON, ON_MOVED_TO_CASH,
                                  ON_MOVED_TO_CASH_WITH_COUPON, ON_SEARCH_FAILED, ON_WAITING, ON_ASSIGNED,
                                  LOCAL_SEND_LEGAL, LOCAL_CALL_TO_DRIVER, LOCAL_CALL_TO_SUPPORT, LOCAL_WHO_IS_TRANSPORTER,
                                  LOCAL_ADD_PHONE_IN_PASSPORT};

class TTaxiPush : public IHandlerGenerator {
public:
    TResultValue Generate(THandler& holder, TApiSchemeHolder scheme) final override;
};

} // namespace NBASS::NPushNotification

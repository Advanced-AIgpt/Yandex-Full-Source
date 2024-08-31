#pragma once

#include "handler.h"

namespace NBASS::NPushNotification {
class TEntitySearchPush: public IHandlerGenerator {
public:
    static constexpr TStringBuf EVENT_BUY_TICKET = "buy_ticket";
    TResultValue Generate(THandler& holder, TApiSchemeHolder scheme) final override;
};
} // namespace NBASS::NPushNotification

#pragma once

#include "checkout_user_info.h"
#include "types.h"

#include <util/generic/hash.h>
#include <util/generic/fwd.h>

namespace NBASS {

namespace NMarket {

class TFakeUsers {
    struct TFakeCheckoutUserInfo
    {
        TFakeCheckoutUserInfo(bool allowOrders, bool allowOnlyPickup)
            : AllowOrders(allowOrders)
            , AllowOnlyPickup(allowOnlyPickup)
        {
        }

        bool AllowOrders;
        bool AllowOnlyPickup;
    };

public:
    static bool IsFakeCheckoutUser(TStringBuf email);
    static bool IsAllowedCheckout(const TCheckoutState& state, const TCheckoutUserInfo& userInfo);

private:
    static THashMap<TString, TFakeCheckoutUserInfo> ReadUsers();
    static const THashMap<TString, TFakeCheckoutUserInfo>& GetUsers();
};
} // namespace NMarket

} // namespace NBASS

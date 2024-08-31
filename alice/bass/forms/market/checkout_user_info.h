#pragma once

#include "context.h"
#include <alice/bass/forms/geo_resolver.h>
#include <alice/bass/forms/market/client/bool_scheme_traits.h>
#include <alice/bass/forms/market/client/bool_scheme_traits.h>
#include <alice/bass/forms/market/client/checkout.sc.h>

#include <alice/bass/forms/context/fwd.h>
#include <alice/bass/forms/geo_resolver.h>

namespace NBASS {

namespace NMarket {

class TCheckoutUserInfo {
public:
    explicit TCheckoutUserInfo(TMarketContext& ctx);

    void Init(bool fullUpdate = false);

    bool IsGuest() const;

    bool HasEmail() const;
    bool HasPhone() const;
    bool HasAddress() const; // метод оставлен для сценария "повторная покупка"
    bool HasLastAddressDelivery() const;
    bool HasLastPickupDelivery() const;

    TAddressSchemeConst GetAddress() const; // метод оставлен для сценария "повторная покупка"
    TAddressSchemeConst GetLastAddress() const;
    TAddressSchemeConst GetLastPickupAddress() const;
    i64 GetLastPickupOutletId() const;
    TStringBuf GetLastPickupOutletName() const;
    i64 GetDeliveryRegionId() const;
    TString GetUid() const;
    TString GetEmail() const;
    TString GetPhone() const;
    TString GetLastName() const;
    TString GetFirstName() const;
    TString GetRecipientName() const;
    TString GetLogin() const;
    bool HasYandexPlus() const;

private:
    TMarketContext& Ctx;
    NBassApi::TMarketCheckoutState<TBoolSchemeTraits> State;

    TString Uid;
    TString Email;
    TString Phone;
    bool YandexPlus;
    NSc::TValue LastAddress;
    NSc::TValue LastPickupAddress;
    i64 LastPickupOutletId;
    TString LastPickupOutletName;
    NSc::TValue Address;

    TString LastName;
    TString FirstName;
};

} // namespace NMarket

} // namespace NBASS

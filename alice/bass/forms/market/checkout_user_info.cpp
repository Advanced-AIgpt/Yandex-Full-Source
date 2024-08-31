#include "checkout_user_info.h"

#include "market_exception.h"
#include "settings.h"
#include "util/string.h"

#include <alice/bass/forms/common/personal_data.h>
#include <alice/bass/forms/context/context.h>
#include <alice/bass/forms/market/client/checkouter_client.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <util/string/strip.h>

#include <regex>

namespace NBASS {

namespace NMarket {

TCheckoutUserInfo::TCheckoutUserInfo(TMarketContext& ctx)
    : Ctx(ctx)
    , State(Ctx.GetCheckoutState())
    , YandexPlus(false)
{
}

void TCheckoutUserInfo::Init(bool fullUpdate /* = false */)
{
    if (IsGuest() && !fullUpdate) {
        return;
    }

    TPersonalDataHelper::TUserInfo blackboxInfo;
    TPersonalDataHelper personalData(Ctx.Ctx());
    if (!personalData.GetUserInfo(blackboxInfo) && !fullUpdate) {
        ythrow TMarketException(TStringBuf("Can not get personal data (it is necessary for this scenario)"));
    }

    TCheckouterClient checkouterClient(Ctx);

    if (blackboxInfo.GetUid().empty() || blackboxInfo.GetEmail().empty()) {
        if (Ctx.GetScenarioType() != EScenarioType::RECURRING_PURCHASE) {
            auto muid = checkouterClient.Auth(Ctx.GetMuid(), Ctx.Meta().ClientIP(), Ctx.MetaClientInfo().UserAgent).GetResponse();
            Ctx.SetMuid(muid);
            State.Muid() = muid.Muid;
        } else {
            Ctx.SetMuid(TMuid());
        }
        return;
    }
    Ctx.DeleteMuid();
    State.Muid() = TStringBuf();

    LOG(INFO) << "Passport UID: " << blackboxInfo.GetUid() << Endl;

    Uid = blackboxInfo.GetUid();
    Email = blackboxInfo.GetEmail();
    YandexPlus = blackboxInfo.GetHasYandexPlus();
    FirstName = blackboxInfo.GetFirstName();
    LastName = blackboxInfo.GetLastName();

    // todo MALISA-240: это не нужно узнавать каждый раз
    const auto& orders = checkouterClient.GetAllOrdersByUid(
        blackboxInfo.GetUid(), 10 /* pageSize */);

    if (!orders.empty()) {
        const auto& lastOrder = orders[0];
        Phone = lastOrder.GetPhone();
        if (lastOrder.HasBuyerAddress()) {
            Address = lastOrder.GetAddress();
            TAddressScheme(&Address).RegionId() = lastOrder.GetDeliveryRegionId();
        }
        for (const auto& order : orders) {
            if (order.HasBuyerAddress()) {
                LastAddress = order.GetAddress();
                TAddressScheme(&LastAddress).RegionId() = order.GetDeliveryRegionId();
                break;
            }
        }
        for (const auto& order : orders) {
            if (order.HasPickupAddress()) {
                LastPickupAddress = order.GetPickupAddress();
                TAddressScheme(&LastPickupAddress).RegionId() = order.GetDeliveryRegionId();
                LastPickupOutletId = order.GetPickupOutletId();
                LastPickupOutletName = order.GetPickupOutletName();
                break;
            }
        }
    } else {
        Phone = blackboxInfo.GetPhone();
    }
}

bool TCheckoutUserInfo::IsGuest() const
{
    return Ctx.HasMuid() || State.HasMuid() && !State.Muid()->empty();
}

bool TCheckoutUserInfo::HasEmail() const
{
    return !GetEmail().empty();
}

bool TCheckoutUserInfo::HasPhone() const
{
    return !GetPhone().empty();
}

bool TCheckoutUserInfo::HasAddress() const
{
    return !GetAddress().IsNull();
}

bool TCheckoutUserInfo::HasLastAddressDelivery() const
{
    return !GetLastAddress().IsNull();
}

bool TCheckoutUserInfo::HasLastPickupDelivery() const
{
    return !GetLastPickupAddress().IsNull();
}

TAddressSchemeConst TCheckoutUserInfo::GetAddress() const
{
    return State.HasBuyerAddress() ? State.BuyerAddress() : TAddressSchemeConst(&Address);
}

TAddressSchemeConst TCheckoutUserInfo::GetLastAddress() const
{
    return TAddressSchemeConst(&LastAddress);
}

TAddressSchemeConst TCheckoutUserInfo::GetLastPickupAddress() const
{
    return TAddressSchemeConst(&LastPickupAddress);
}

i64 TCheckoutUserInfo::GetLastPickupOutletId() const
{
    return LastPickupOutletId;
}

TStringBuf TCheckoutUserInfo::GetLastPickupOutletName() const
{
    return LastPickupOutletName;
}

i64 TCheckoutUserInfo::GetDeliveryRegionId() const
{
    return GetLastAddress().RegionId();
}

TString TCheckoutUserInfo::GetUid() const
{
    return IsGuest() ? Ctx.GetMuid() ? Ctx.GetMuid()->Muid : TString(State.Muid()) : Uid;
}

TString TCheckoutUserInfo::GetEmail() const
{
    return State.HasEmail() ? TString(State.Email()) : Email;
}

TString TCheckoutUserInfo::GetPhone() const
{
    return State.HasPhone() ? TString(State.Phone()) : Phone;
}

TString TCheckoutUserInfo::GetLastName() const
{
    return LastName.empty() ? TString(GetLogin()) : LastName;
}

TString TCheckoutUserInfo::GetFirstName() const
{
    return FirstName.empty() ? TString(GetLogin()) : FirstName;
}

TString TCheckoutUserInfo::GetRecipientName() const
{
    TStringBuilder result;
    result << FirstName;
    if (!result.empty()) {
        result << " ";
    }
    result << LastName;

    return result.empty() ? TString(GetLogin()) : result;
}

TString TCheckoutUserInfo::GetLogin() const
{
    const auto email = GetEmail();
    return email.substr(0, email.find('@'));
}

bool TCheckoutUserInfo::HasYandexPlus() const
{
    return YandexPlus;
}

} // namespace NMarket

} // namespace NBASS

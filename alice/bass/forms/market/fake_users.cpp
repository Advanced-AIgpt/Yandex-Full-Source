#include "fake_users.h"

#include <library/cpp/resource/resource.h>
#include <library/cpp/scheme/scheme.h>
#include <util/generic/hash.h>

namespace NBASS {

namespace NMarket {

THashMap<TString, TFakeUsers::TFakeCheckoutUserInfo> TFakeUsers::ReadUsers()
{
    const auto fakeUsersData = NSc::TValue::FromJson(NResource::Find(TStringBuf("market_fake_users.txt")));

    THashMap<TString, TFakeCheckoutUserInfo> users(fakeUsersData.GetArray().size());
    for (const auto& userData : fakeUsersData.GetArray()) {
        users.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(userData["email"].ForceString()),
            std::forward_as_tuple(
                userData["allow_orders"].GetBool(true),
                userData["allow_only_pickup"].GetBool(false)));
    }
    return users;
}

const THashMap<TString, TFakeUsers::TFakeCheckoutUserInfo>& TFakeUsers::GetUsers()
{
    static const THashMap<TString, TFakeCheckoutUserInfo> users = ReadUsers();
    return users;
}

bool TFakeUsers::IsFakeCheckoutUser(TStringBuf email)
{
    return GetUsers().contains(email);
}

bool TFakeUsers::IsAllowedCheckout(const TCheckoutState& state, const TCheckoutUserInfo& userInfo)
{
    if (!GetUsers().contains(userInfo.GetEmail())) {
        return true;
    }
    const auto& fakeUserInfo = GetUsers().at(userInfo.GetEmail());
    if (!fakeUserInfo.AllowOrders) {
        return false;
    }
    if (state.Delivery().Type() != TStringBuf("PICKUP")) {
        return !fakeUserInfo.AllowOnlyPickup;
    }
    return true;
}

} // namespace NMarket

} // namespace NBASS

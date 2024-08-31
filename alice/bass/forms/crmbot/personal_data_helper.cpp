#include "personal_data_helper.h"

namespace {

const NSc::TValue& GetPersonalData(const NBASS::TContext& ctx) {
    return ctx.Meta().HasPersonalData() ? *ctx.Meta().PersonalData().GetRawValue() : NSc::Null();
}

}

namespace NBASS::NCrmbot {

TPersonalDataHelper::TPersonalDataHelper(const NBASS::TContext& ctx)
    : PersonalData(GetPersonalData(ctx)) {
}

TMaybe<i64> TPersonalDataHelper::GetRegionId() const
{
    if (PersonalData.Has("region_id")) {
        return PersonalData.Get("region_id").GetIntNumber();
    } else {
        return Nothing();
    }
}

TStringBuf TPersonalDataHelper::GetName() const
{
    return PersonalData.Get("name").GetString();
}

TStringBuf TPersonalDataHelper::GetEmail() const
{
    return PersonalData.Get("email").GetString();
}

TStringBuf TPersonalDataHelper::GetPhone() const
{
    return PersonalData.Get("phone").GetString();
}

TStringBuf TPersonalDataHelper::GetIP() const
{
    if (PersonalData.Has("secure")) {
        return PersonalData["secure"].Get("ip").GetString();
    } else return TStringBuf();
}

TStringBuf TPersonalDataHelper::GetYandexUid() const
{
    if (PersonalData.Has("secure")) {
        return PersonalData["secure"].Get("yandexuid").GetString();
    } else return TStringBuf();
}

TStringBuf TPersonalDataHelper::GetPuid() const
{
    if (PersonalData.Has("secure")) {
        return PersonalData["secure"].Get("uid").GetString();
    } else return TStringBuf();
}

TStringBuf TPersonalDataHelper::GetMuid() const
{
    if (PersonalData.Has("secure")) {
        return PersonalData["secure"].Get("muid").GetString();
    } else return TStringBuf();
}

}

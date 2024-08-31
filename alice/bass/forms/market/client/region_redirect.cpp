#include <alice/bass/forms/market/client.h>
#include <alice/bass/forms/market/util/string.h>

namespace NBASS {

namespace NMarket {

TReportResponse::TRegionRedirect::TRegionRedirect(NSc::TValue data)
    : TBaseRedirect(data)
    , UserRegion(0)
{
    NSc::TValue& redirect = Data["redirect"];
    const auto& params = redirect["params"];
    UserRegion = FromStringWithLogging<i64>(params["lr"][0].GetString(), 0);
}

i64 TReportResponse::TRegionRedirect::GetUserRegion() const
{
    return UserRegion;
}

} // namespace NMarket

} // namespace NBASS

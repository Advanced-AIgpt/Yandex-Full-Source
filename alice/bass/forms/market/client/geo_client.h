#pragma once

#include "bool_scheme_traits.h"
#include "base_client.h"
#include <alice/bass/forms/market/types.h>

#include <library/cpp/scheme/domscheme_traits.h>
#include <alice/bass/forms/market/checkout_user_info.h>
#include <alice/bass/forms/market/client/checkout.sc.h>
#include <alice/bass/forms/market/client/report.sc.h>

namespace NBASS {

namespace NMarket {

TMaybe<TSchemeHolder<TAddressScheme>> RequestAddressResolution(
    const TMarketContext& ctx,
    TStringBuf searchText,
    TMaybe<NBASS::TGeoPosition> geoPosition);

} // namespace NMarket

} // namespace NBASS

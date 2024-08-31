#pragma once

#include <alice/bass/forms/market/types.h>

#include <library/cpp/scheme/scheme.h>
#include <library/cpp/cgiparam/cgiparam.h>

namespace NBASS {

namespace NMarket {

NSc::TValue GetFiltersForDetails(const NSc::TArray& reportFilters);
void SetRedirectModeCgiParam(bool allowRedirects, TCgiParameters& cgi);
void SetRedirectCgiParams(const TRedirectCgiParams& redirectParams, TCgiParameters& cgi);
void SetPriceCgiParams(const NSc::TValue& price, TCgiParameters& cgi);
void AddGlFilters(const TCgiGlFilters& glFilters, TCgiParameters& cgi);

TString FormatPof(ui32 clid);

} // namespace NMarket

} // namespace NBASS

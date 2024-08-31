#pragma once

#include <alice/library/util/status.h>

#include <util/generic/string.h>
#include <util/generic/variant.h>

namespace NAlice::NBilling {

namespace NImpl {

TVector<TString> FilterBillingExperiments(const THashMap<TString, TMaybe<TString>>& expFlags);

TString ExpFlagsToJson(const TVector<TString>& expFlags);

} // namespace NImpl

TString AppHostRequestPromoUrlPath(bool sendPush = false, bool sendPersonalCards = false);

TMaybe<TString> ExpFlagsToBillingHeader(const THashMap<TString, TMaybe<TString>>& expFlags);

struct TPromoAvailability {
    bool IsAvailable = false;
    TString ActivatePromoUri = "";
    TString ExtraPeriodExpiresDate = "";
};

enum class EBillingErrorCode {
    ACTIVATE_PROMO_URI_ERROR /* "activate_promo_uri_error" */,
    EXPIRATION_DATE_ERROR /* "expiration_date_error" */,
    STATUS_ERROR /* "status_error" */,
};

using TBillingError = NAlice::TGenericError<EBillingErrorCode>;

template<typename T>
using TBillingErrorOr = std::variant<TBillingError, T>;

TBillingErrorOr<TPromoAvailability> ParseBillingResponse(const TStringBuf responseJsonStr, const bool needActivatePromoUri = true);

} // namespace NAlice::NBilling

#include "billing.h"

#include <alice/library/billing/defs.h>
#include <alice/library/json/json.h>

#include <util/draft/date.h>

namespace NAlice::NBilling {

namespace {

const TString ACTIVATE_PROMO_URI = "activate_promo_uri";

const TString BILLING_EXP_PREFIX = "set_billing_exp=";

TString BoolToString(bool value) {
    return value ? "true" : "false";
}

bool ExtraPeriodExpiresDateIsCorrect(const TString& expiresDateStr) {
    try {
        TDate(expiresDateStr, "%Y-%m-%d");
        return true;
    } catch (const yexception& e) {
        return false;
    }
}

} // namespace

namespace NImpl {

TVector<TString> FilterBillingExperiments(const THashMap<TString, TMaybe<TString>>& expFlags) {
    TVector<TString> billingFlags;
    for (const auto& [expFlag, _] : expFlags) {
        if (expFlag.StartsWith(BILLING_EXP_PREFIX)) {
            billingFlags.push_back(expFlag.substr(BILLING_EXP_PREFIX.size()));
        }
    }
    return billingFlags;
}

TString ExpFlagsToJson(const TVector<TString>& expFlags) {
    auto expFlagsJson = NAlice::JsonFromString(
        "[{"
            "\"HANDLER\":\"VOICE\","
            "\"CONTEXT\":{\"MAIN\":{\"VOICE\":{\"flags\":[]}}}"
        "}]"
    );
    auto& flags = expFlagsJson[0]["CONTEXT"]["MAIN"]["VOICE"]["flags"];
    for (const auto& expFlag : expFlags) {
        flags.AppendValue(expFlag);
    }
    return NAlice::JsonToString(expFlagsJson);
}

} // namespace NImpl

TString AppHostRequestPromoUrlPath(bool sendPush, bool sendPersonalCards) {
    return TString::Join("/requestPlus?sendPush=", BoolToString(sendPush),
                         "&sendPersonalCards=", BoolToString(sendPersonalCards));
}

TMaybe<TString> ExpFlagsToBillingHeader(const THashMap<TString, TMaybe<TString>>& expFlags) {
    const auto billingExpFlags = NImpl::FilterBillingExperiments(expFlags);
    if (!billingExpFlags.empty()) {
        return NImpl::ExpFlagsToJson(billingExpFlags);
    }
    return Nothing();
}

TBillingErrorOr<TPromoAvailability> ParseBillingResponse(const TStringBuf responseJsonStr, const bool needActivatePromoUri) {
    TPromoAvailability result{false /* IsAvailable */, "" /* ActivatePromoUri */, "" /* ExtraPeriodExpiresDate */};
    const auto response = JsonFromString(responseJsonStr);
    const auto& responseResult = response["result"];

    if (responseResult == "PAYMENT_REQUIRED") {
        return result;
    } else if (responseResult == "PROMO_AVAILABLE") {
        result.IsAvailable = true;
        if (needActivatePromoUri) {
            if (response[ACTIVATE_PROMO_URI].IsString()) {
                result.ActivatePromoUri = response[ACTIVATE_PROMO_URI].GetStringSafe();
            } else {
                return TBillingError(EBillingErrorCode::EXPIRATION_DATE_ERROR) << "activate_promo_uri is required";
            }
        }
        if (response[EXTRA_PROMO_PERIOD_EXPIRES_DATE_BILLING].IsString()) {
            const TString expiresDateStr = response[EXTRA_PROMO_PERIOD_EXPIRES_DATE_BILLING].GetStringSafe();
            const bool inExtraPromoBillingExp = (response["experiment"].IsString() &&
                                                 response["experiment"].GetStringSafe() == "plus120");
            if (inExtraPromoBillingExp && ExtraPeriodExpiresDateIsCorrect(expiresDateStr)) {
                result.ExtraPeriodExpiresDate = expiresDateStr;
            } else if (inExtraPromoBillingExp) {
                return TBillingError(EBillingErrorCode::EXPIRATION_DATE_ERROR) << "Incorrect extra period expiration date: " << expiresDateStr;
            }
        }
        return result;
    } else {
        return TBillingError(EBillingErrorCode::STATUS_ERROR) << "Unexpected billing status: " << responseResult;
    }

}

} // namespace NAlice::NBilling

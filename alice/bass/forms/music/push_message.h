#pragma once

#include <alice/bass/forms/context/context.h>

#include <alice/library/billing/billing.h>

namespace NBASS::NMusic {

struct TPushMessageParams {
    TString Id;
    TString Tag;
    TString Text;
    TString ThrottlePolicy;
    TString Title;
    TString Url;
};

void AddPushMessageResponseDirective(TContext& ctx, const TPushMessageParams& pushMessageParams);

void AddPromoSendPushDirective(TContext& ctx, NAlice::NBilling::TPromoAvailability promoAvailability);

}

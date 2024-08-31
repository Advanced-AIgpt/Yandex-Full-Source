#pragma once

#include "base_handler.h"

#include <alice/bass/forms/crmbot/clients/checkouter.h>

namespace NBASS::NCrmbot {

class TCrmbotContext;

class TAuthorizingOrderFormHandler : public TCrmbotFormHandler {
public:
    TResultValue Do(TRequestHandler& r) override;

protected:
    virtual void ChangeFormToScenarioEntry(TCrmbotContext& ctx) const = 0;

    virtual void MakeSingleOrderResponse(TCrmbotContext& ctx, const TCheckouterOrder& order) const = 0;
    virtual void MakeOrderListResponse(TCrmbotContext& ctx, TStringBuf uid) const = 0;

private:
    void HandleAnonymous(TCrmbotContext& ctx) const;
    void HandleAuthorized(TCrmbotContext& ctx, TStringBuf puid, TStringBuf muid) const;

    TString FormatPhoneNumber(TStringBuf number) const;
    TString FormatEmail(TStringBuf email) const;

    bool CheckEmail(TStringBuf email) const;
    bool CheckPhoneNumber(TStringBuf number) const;
    void CheckOrderPhoneAndEmail(TMaybe<TCheckouterOrder>& order, TStringBuf email, TStringBuf phone) const;

    bool TryFillOrderId(TStringBuf val, TCrmbotContext& ctx) const;
    bool TryFillPhoneNumber(TStringBuf val, TCrmbotContext& ctx) const;
    bool TryFillEmail(TStringBuf val, TCrmbotContext& ctx) const;

    void TryFillSlots(TCrmbotContext& ctx) const;
};

class TListingAuthorizingOrderFormHandler : public TAuthorizingOrderFormHandler {
protected:
    void MakeOrderListResponse(TCrmbotContext& ctx, TStringBuf uid) const override;
};

}
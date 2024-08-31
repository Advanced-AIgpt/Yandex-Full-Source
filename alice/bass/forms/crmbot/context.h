#pragma once

#include <alice/bass/forms/market/context.h>

namespace NBASS::NCrmbot {

class TCrmbotContext : public NBASS::NMarket::TMarketContext {
public:
    explicit TCrmbotContext(TContext& ctx) : TMarketContext(ctx)
    {
    }

    bool IsWebim() const {
        return Ctx().Meta()->ClientInfo()->AppId() == TStringBuf("webim_crm_bot");
    }
    bool IsWebim1() const {
        return IsWebim() && Ctx().Meta()->ClientInfo()->AppVersion() == TStringBuf("1.0");
    }
    bool IsWebim2() const {
        return IsWebim() && Ctx().Meta()->ClientInfo()->AppVersion() == TStringBuf("2.0");
    }
};

}

#include "login.h"

namespace NBASS {

namespace NMarket {

TResultValue HandleGuest(TMarketContext& ctx, bool isLoginForm) {
    if (isLoginForm) {
        ctx.AddTextCardBlock(TStringBuf("market_common__still_no_login"));
    } else {
        ctx.AddTextCardBlock(TStringBuf("market_common__no_login"));
    }
    ctx.AddAuthorizationSuggest();
    if (!ctx.Ctx().MetaClientInfo().IsNavigator()) {
        ctx.AddSuggest(TStringBuf("market_common__user_logined"));
    }
    return TResultValue();
}

} // namespace NMarket

} // namespace NBASS

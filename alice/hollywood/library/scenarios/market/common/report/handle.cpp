#include "handle.h"

#include "proxy.h"

namespace NAlice::NHollywood::NMarket {

TReportResponseHandleImpl::TReportResponseHandleImpl(
        TScenarioHandleContext& ctx,
        bool allowRedirects)
    : Ctx(ctx)
    , AllowRedirects(allowRedirects)
{}

void TReportResponseHandleImpl::Do()
{
    auto searchInfo = GetSearchInfoOrThrow(Ctx);
    auto client = GetReportClientOrThrow(Ctx);
    const auto response = RetireReportPrimeResponse(Ctx);

    LOG_INFO(Ctx.Logger())
        << "is redirect - " << response.IsRedirect()
        << ", allow redirects - " << AllowRedirects;

    if (response.IsRedirect()) {
        const auto& redirect = response.GetRedirect();
        Ctx->ServiceCtx.AddFlag(TStringBuf("redirect"));
        searchInfo.ApplyRedirect(redirect);
        AddSearchInfoRequest(
            searchInfo,
            client,
            Ctx,
            AllowRedirects && redirect.HasRedirect);
    }
}

} // namespace NAlice::NHollywood::NMarket

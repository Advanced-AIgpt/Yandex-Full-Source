#include "pers_basket_client.h"

#include <alice/bass/forms/market/context.h>

namespace NBASS {

namespace NMarket {

TPersBasketClient::TPersBasketClient(TMarketContext& ctx)
    : TBaseClient(ctx.GetSources(), ctx)
{
}

THttpResponse<TPersBasketEntryResponse> TPersBasketClient::GetAllEnties(TStringBuf uid)
{
    const auto& source = Sources.MarketPersBasket();

    TCgiParameters cgi = CreateCheckoutCgiParams(uid);

    return Run<TPersBasketEntryResponse>(source, cgi).Wait();
}

TCgiParameters TPersBasketClient::CreateCheckoutCgiParams(TStringBuf uid) const
{
    TCgiParameters cgi;
    cgi.InsertUnescaped(TStringBuf("userIdType"), TStringBuf("UID"));
    cgi.InsertUnescaped(TStringBuf("userAnyId"), uid);
    return cgi;
}

THttpResponse<void> TPersBasketClient::AddEntry(TStringBuf uid, const TPersBasketEntry& entry)
{
    const auto& source = Sources.MarketPersBasket();

    TCgiParameters cgi = CreateCheckoutCgiParams(uid);

    NSc::TValue body;

    body.SetArray().Push() = entry.ToJson();

    return Run<void>(source, cgi, body).Wait();
}

THttpResponse<void> TPersBasketClient::AddEntries(TStringBuf uid, const TVector<TPersBasketEntry>& entries)
{
    const auto& source = Sources.MarketPersBasket();

    TCgiParameters cgi = CreateCheckoutCgiParams(uid);

    NSc::TValue body;
    body.SetArray();
    for (const auto& entry : entries) {
        body.Push() = entry.ToJson();
    }

    return Run<void>(source, cgi, body).Wait();
}

THttpResponse<void> TPersBasketClient::DeleteEntry(TStringBuf uid, TPersBasketEntryId entryId)
{
    return DeleteEntryAsync(uid, entryId).Wait();
}

TResponseHandle<void> TPersBasketClient::DeleteEntryAsync(TStringBuf uid, TPersBasketEntryId entryId)
{
    const auto& source = Sources.MarketPersBasket(TStringBuilder() << '/' << entryId);

    TCgiParameters cgi = CreateCheckoutCgiParams(uid);

    return Run<void>(source, cgi, NSc::TValue(), THashMap<TString, TString>(), TStringBuf("DELETE"));
}

} // namespace NMarket

} // namespace NBASS

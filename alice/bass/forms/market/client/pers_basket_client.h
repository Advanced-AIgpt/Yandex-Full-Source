#pragma once

#include "base_client.h"
#include "pers_basket_entry.h"
#include <alice/bass/forms/market/types.h>

#include <library/cpp/scheme/domscheme_traits.h>

namespace NBASS {

namespace NMarket {

class TPersBasketClient: public TBaseClient {
public:
    explicit TPersBasketClient(TMarketContext& context);

    THttpResponse<TPersBasketEntryResponse> GetAllEnties(TStringBuf uid);

    THttpResponse<void> AddEntry(TStringBuf uid, const TPersBasketEntry& entry);
    THttpResponse<void> AddEntries(TStringBuf uid, const TVector<TPersBasketEntry>& entries);

    THttpResponse<void> DeleteEntry(TStringBuf uid, TPersBasketEntryId entryId);
    TResponseHandle<void> DeleteEntryAsync(TStringBuf uid, TPersBasketEntryId entryId);

private:
    TCgiParameters CreateCheckoutCgiParams(TStringBuf uid) const;

};

} // namespace NMarket

} // namespace NBASS

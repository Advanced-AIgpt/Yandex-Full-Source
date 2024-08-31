#pragma once

#include <alice/bass/forms/context/context.h>
#include <util/system/types.h>

namespace NBASS {

namespace NMarket {

class TMarketContext;

void AddMarketSuggests(TMarketContext& ctx);
void AddOnboardingSuggestIfNeeded(TMarketContext& ctx);
void AddCancelSuggest(TMarketContext& ctx);
void DeleteCancelSuggestIfNeeded(TMarketContext& ctx);
void AddStartChoiceAgainSuggest(TMarketContext& ctx);
void AddProductDetailsCardSuggests(TMarketContext& ctx, bool isBlueOffer, bool hasShopUrl);
void AddBeruProductDetailsCardSuggest(TMarketContext& ctx);
void AddBeruOrderCardSuggests(TMarketContext& ctx);
void AddNumberSuggests(TMarketContext& ctx, size_t count);
void AddConfirmCheckoutSuggests(TMarketContext& ctx);
void AddConfirmSuggests(TMarketContext& ctx);
void AddCheckoutWaitSuggests(TMarketContext& ctx);
void AddUserLoginedSuggest(TMarketContext& ctx);
void AddCartSuggest(TMarketContext& ctx, ui64 sku);

} // End of namespace NMarket

} // End of namespace NBass

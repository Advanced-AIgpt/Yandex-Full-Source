#include "suggests.h"

#include <alice/bass/forms/market/bass_response.sc.h>
#include <alice/bass/forms/market/context.h>
#include <alice/bass/forms/market/forms.h>
#include <alice/bass/forms/market/experiments.h>
#include <alice/bass/forms/market/state.h>

namespace NBASS {

namespace NMarket {

void AddMarketSuggests(TMarketContext& ctx)
{
    AddCancelSuggest(ctx);
    AddStartChoiceAgainSuggest(ctx);
    AddOnboardingSuggestIfNeeded(ctx);
}

void AddOnboardingSuggestIfNeeded(TMarketContext& ctx)
{
    if (ctx.GetState(EChoiceState::Null) == EChoiceState::CheckoutComplete) {
        ctx.AddOnboardingSuggest();
    }
}

void AddCancelSuggest(TMarketContext& ctx)
{
    ctx.AddSuggest(TStringBuf("market__cancel"));
}

void DeleteCancelSuggestIfNeeded(TMarketContext& ctx)
{
    if (EqualToOneOf(ctx.GetState(EChoiceState::Null),
            EChoiceState::Exit, EChoiceState::CheckoutComplete,
            EChoiceState::ChoiceOpen, EChoiceState::ActivationOpen,
            EChoiceState::MakeOrderOpen, EChoiceState::ProductDetailsOpen,
            EChoiceState::BeruProductDetailsOpen, EChoiceState::ProductDetailsExternal))
    {
        ctx.DeleteSuggest(TStringBuf("market__cancel"));
    }
}

void AddStartChoiceAgainSuggest(TMarketContext& ctx)
{
    switch (ctx.GetState(EChoiceState::Null)) {
        case EChoiceState::Activation:
        case EChoiceState::ActivationOpen:
        case EChoiceState::Exit:
        case EChoiceState::ProductDetailsExternal:
            break;
        case EChoiceState::Null:
        case EChoiceState::Choice:
        case EChoiceState::ProductDetails:
        case EChoiceState::BeruProductDetails:
        case EChoiceState::MakeOrder:
        case EChoiceState::ChoiceOpen:
        case EChoiceState::ProductDetailsOpen:
        case EChoiceState::BeruProductDetailsOpen:
        case EChoiceState::MakeOrderOpen:
        case EChoiceState::CheckoutItemsNumber:
        case EChoiceState::CheckoutPhone:
        case EChoiceState::CheckoutEmail:
        case EChoiceState::CheckoutAddress:
        case EChoiceState::CheckoutDeliveryOptions:
        case EChoiceState::CheckoutConfirmOrder:
        case EChoiceState::CheckoutWaiting:
        case EChoiceState::CheckoutComplete:
            const auto form = FromString<EChoiceForm>(ctx.FormName());
            static const THashSet<EChoiceForm> formsNotForChoiceAgain{
                EChoiceForm::Activation,
                EChoiceForm::BeruActivation,
                EChoiceForm::StartChoiceAgain,
            };
            if (!formsNotForChoiceAgain.contains(form)) {
                NSc::TValue formUpdateData;
                NBassApi::TFormUpdate<TSchemeTraits> formUpdate(&formUpdateData);
                formUpdate.Name() = ToString(EChoiceForm::StartChoiceAgain);
                ctx.AddSuggest(TStringBuf("market__start_choice_again"), NSc::TValue(), formUpdateData);
            }
    }
}

void AddProductDetailsCardSuggests(TMarketContext& ctx, bool isBlueOffer, bool hasShopUrl)
{
    if (isBlueOffer) {
        ctx.AddSuggest(TStringBuf("market__product_details__beru"));
    }
    if (hasShopUrl) {
        ctx.AddSuggest(TStringBuf("market__go_to_shop"));
    }
}

void AddBeruProductDetailsCardSuggest(TMarketContext& ctx)
{
    ctx.AddSuggest(TStringBuf("market__beru_product_details"));
}

void AddBeruOrderCardSuggests(TMarketContext& ctx)
{
    if (ctx.FormName() != ToString(EChoiceForm::ProductDetailsExternal)) {
        ctx.AddSuggest(TStringBuf("market__beru_order"));
    } else {
        ctx.AddSuggest(TStringBuf("market__product_details__order_witch_alice"));
    }
}

void AddNumberSuggests(TMarketContext& ctx, size_t count)
{
    for (size_t i = 1; i <= count; ++i) {
        ctx.AddSuggest(TStringBuf("market__number"), NSc::TValue(ToString<size_t>(i)));
    }
}

void AddConfirmCheckoutSuggests(TMarketContext& ctx)
{
    ctx.AddSuggest(TStringBuf("market__confirm_checkout__yes"));
    ctx.AddSuggest(TStringBuf("market__confirm_checkout__no"));
}

void AddConfirmSuggests(TMarketContext& ctx)
{
    ctx.AddSuggest(TStringBuf("market__confirm__yes"));
    ctx.AddSuggest(TStringBuf("market__confirm__no"));
}

void AddCheckoutWaitSuggests(TMarketContext& ctx)
{
    ctx.AddSuggest(TStringBuf("market__checkout_wait__good"));
    ctx.AddSuggest(TStringBuf("market__checkout_wait__bad"));
}

void AddUserLoginedSuggest(TMarketContext& ctx)
{
    ctx.AddSuggest(TStringBuf("market__user_logined"));
    ctx.AddAuthorizationSuggest();
}

void AddCartSuggest(TMarketContext& ctx, ui64 sku)
{
    ctx.AddSuggest(TStringBuf("market__add_to_cart"), NSc::TValue(), ctx.GetAddToCartFormUpdate(sku).Value());
}

} // End of namespace NMarket

} // End of namespace NBass

#pragma once

#include <util/generic/strbuf.h>

namespace NBASS {

namespace NMarket {

enum class EHowMuchForm {
    HowMuch /* "personal_assistant.scenarios.how_much" */,
    HowMuchEllipsis /* "personal_assistant.scenarios.how_much__ellipsis" */,
};

enum class EChoiceForm {
    Native /* "personal_assistant.scenarios.market_native" */,
    NativeBeru /* "personal_assistant.scenarios.market_native_beru" */,
    Activation /* "personal_assistant.scenarios.market" */,
    BeruActivation /* "personal_assistant.scenarios.market_beru" */,
    Cancel /* "personal_assistant.scenarios.market__cancel" */,
    Garbage /* "personal_assistant.scenarios.market__garbage" */,
    Repeat /* "personal_assistant.scenarios.market__repeat" */,
    ShowMore /* "personal_assistant.scenarios.market__show_more" */,
    StartChoiceAgain /* "personal_assistant.scenarios.market__start_choice_again" */,
    MarketChoice /* "personal_assistant.scenarios.market__market" */,
    MarketChoiceEllipsis /* "personal_assistant.scenarios.market__market__ellipsis" */,
    MarketParams /* "personal_assistant.scenarios.market__market__params" */,
    MarketNumberFilter /* "personal_assistant.scenarios.market__number_filter" */,
    ProductDetails /* "personal_assistant.scenarios.market__product_details" */,
    BeruOrder /* "personal_assistant.scenarios.market__beru_order" */,
    AddToCart /* "personal_assistant.scenarios.market__add_to_cart" */,
    GoToShop /* "personal_assistant.scenarios.market__go_to_shop" */,

    ProductDetailsExternal /* "personal_assistant.scenarios.market_product_details" */,
    ProductDetailsExternal_BeruOrder /* "personal_assistant.scenarios.market_product_details__beru_order" */,

    Checkout /* "personal_assistant.scenarios.market__checkout" */,
    CheckoutItemsNumber /* "personal_assistant.scenarios.market__checkout_items_number" */,
    CheckoutEmail /* "personal_assistant.scenarios.market__checkout_email" */,
    CheckoutPhone /* "personal_assistant.scenarios.market__checkout_phone" */,
    CheckoutAddress /* "personal_assistant.scenarios.market__checkout_address" */,
    CheckoutIndex /* "personal_assistant.scenarios.market__checkout_index" */,
    CheckoutDeliveryIntervals /* "personal_assistant.scenarios.market__checkout_delivery_intervals" */,
    CheckoutYesOrNo /* "personal_assistant.scenarios.market__checkout_yes_or_no" */,
    CheckoutEverything /* "personal_assistant.scenarios.market__checkout_everything" */,
};

enum class ERecurringPurchaseForm {
    Activation /* "personal_assistant.scenarios.recurring_purchase" */,
    Login /* "personal_assistant.scenarios.recurring_purchase__login" */,
    Cancel /* "personal_assistant.scenarios.recurring_purchase__cancel" */,
    Garbage /* "personal_assistant.scenarios.recurring_purchase__garbage" */,
    Repeat /* "personal_assistant.scenarios.recurring_purchase__repeat" */,
    Ellipsis /* "personal_assistant.scenarios.recurring_purchase__ellipsis" */,
    NumberFilter /* "personal_assistant.scenarios.recurring_purchase__number_filter" */,
    ProductDetails /* "personal_assistant.scenarios.recurring_purchase__product_details" */,
    Checkout /* "personal_assistant.scenarios.recurring_purchase__checkout" */,
    CheckoutItemsNumber /* "personal_assistant.scenarios.recurring_purchase__checkout_items_number" */,
    CheckoutIndex /* "personal_assistant.scenarios.recurring_purchase__checkout_index" */,
    CheckoutYesOrNo /* "personal_assistant.scenarios.recurring_purchase__checkout_yes_or_no" */,
    CheckoutSuits /* "personal_assistant.scenarios.recurring_purchase__checkout_suits" */,
    CheckoutDeliveryIntervals /* "personal_assistant.scenarios.recurring_purchase__checkout_delivery_intervals" */,
    CheckoutEverything /* "personal_assistant.scenarios.recurring_purchase__checkout_everything" */,
};

enum class EMarketOrdersStatusForm {
    MarketOrdersStatus /* "personal_assistant.scenarios.market_orders_status" */,
    Login /* "personal_assistant.scenarios.market_orders_status__login" */,
};

enum class EMarketBeruBonusesForm {
    MyBonusesList /* "personal_assistant.scenarios.market_beru_my_bonuses_list" */,
    Login /* "personal_assistant.scenarios.market_beru_my_bonuses_list__login" */,
};

enum class EShoppingListForm {
    Add /* "personal_assistant.scenarios.shopping_list_add" */,
    Show /* "personal_assistant.scenarios.shopping_list_show" */,
    Show_Show /* "personal_assistant.scenarios.shopping_list_show__show" */,
    Show_Add /* "personal_assistant.scenarios.shopping_list_show__add" */,
    Show_DeleteAll /* "personal_assistant.scenarios.shopping_list_show__delete_all" */,
    Show_DeleteIndex /* "personal_assistant.scenarios.shopping_list_show__delete_index" */,
    Show_DeleteItem /* "personal_assistant.scenarios.shopping_list_show__delete_item" */,
    DeleteItem /* "personal_assistant.scenarios.shopping_list_delete_item" */,
    DeleteAll /* "personal_assistant.scenarios.shopping_list_delete_all" */,
    Login /* "personal_assistant.scenarios.shopping_list_login" */,

    AddFixlist /* "personal_assistant.scenarios.shopping_list_add_fixlist" */,
    ShowFixlist /* "personal_assistant.scenarios.shopping_list_show_fixlist" */,
    Show_ShowFixlist /* "personal_assistant.scenarios.shopping_list_show__show_fixlist" */,
    Show_AddFixlist /* "personal_assistant.scenarios.shopping_list_show__add_fixlist" */,
    Show_DeleteAllFixlist /* "personal_assistant.scenarios.shopping_list_show__delete_all_fixlist" */,
    Show_DeleteIndexFixlist /* "personal_assistant.scenarios.shopping_list_show__delete_index_fixlist" */,
    Show_DeleteItemFixlist /* "personal_assistant.scenarios.shopping_list_show__delete_item_fixlist" */,
    DeleteItemFixlist /* "personal_assistant.scenarios.shopping_list_delete_item_fixlist" */,
    DeleteAllFixlist /* "personal_assistant.scenarios.shopping_list_delete_all_fixlist" */,
    LoginFixlist /* "personal_assistant.scenarios.shopping_list_login_fixlist" */,
};


} // namespace NMarket

} // namespace NBASS

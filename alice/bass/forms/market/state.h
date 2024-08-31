#pragma once

namespace NBASS {

namespace NMarket {

enum class EChoiceState {
    Null /* "" */,
    Activation /* "Market.Choice.Activation" */,
    Choice /* "Market.Choice.Choice" */,
    ProductDetails /* "Market.Choice.ProductDetails" */,
    BeruProductDetails /* "Market.Choice.BeruProductDetails" */,
    MakeOrder /* "Market.Choice.MakeOrder" */,
    ActivationOpen /* "Market.Choice.Activation.Open" */,
    ChoiceOpen /* "Market.Choice.Choice.Open" */,
    ProductDetailsOpen /* "Market.Choice.ProductDetails.Open" */,
    BeruProductDetailsOpen /* "Market.Choice.BeruProductDetails.Open" */,
    MakeOrderOpen /* "Market.Choice.MakeOrder.Open" */,
    CheckoutItemsNumber /* "Market.Choice.CheckoutItemsNumber" */,
    CheckoutEmail /* "Market.Choice.CheckoutEmail" */,
    CheckoutPhone /* "Market.Choice.CheckoutPhone" */,
    CheckoutAddress /* "Market.Choice.CheckoutAddress" */,
    CheckoutDeliveryOptions /* "Market.Choice.CheckoutDeliveryOptions" */,
    CheckoutConfirmOrder /* "Market.Choice.CheckoutConfirmOrder" */,
    CheckoutWaiting /* "Market.Choice.CheckoutWaiting" */,
    CheckoutComplete /* "Market.Choice.CheckoutComplete" */,
    ProductDetailsExternal /* "Market.Choice.ProductDetailsExternal" */,
    Exit /* "Market.Choice.Exit" */,
};

enum class ERecurringPurchaseState {
    Null /* "" */,
    RecurringPurchase /* "Market.RecurringPurchase.RecurringPurchase" */,
    Choice /* "Market.RecurringPurchase.Choice" */,
    Login /* "Market.RecurringPurchase.Login" */,
    SelectProductIndex /* "Market.RecurringPurchase.SelectProductIndex" */,
    ProductDetails /* "Market.RecurringPurchase.ProductDetails" */,
    ProductDetailsScreenless /* "Market.RecurringPurchase.ProductDetailsScreenless" */,
    CheckoutItemsNumber /* "Market.RecurringPurchase.CheckoutItemsNumber" */,
    CheckoutPhone /* "Market.RecurringPurchase.CheckoutPhone" */,
    CheckoutAddress /* "Market.RecurringPurchase.CheckoutAddress" */,
    CheckoutDeliveryOptions /* "Market.RecurringPurchase.CheckoutDeliveryOptions" */,
    CheckoutDeliveryFirstOption /* "Market.RecurringPurchase.CheckoutDeliveryFirstOption" */,
    CheckoutConfirmOrder /* "Market.RecurringPurchase.CheckoutConfirmOrder" */,
    CheckoutWaiting /* "Market.RecurringPurchase.CheckoutWaiting" */,
    CheckoutComplete /* "Market.RecurringPurchase.CheckoutComplete" */,
    Exit /* "Market.RecurringPurchase.Exit" */,
};

} // namespace NMarket

} // namespace NBASS

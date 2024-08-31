package ru.yandex.quasar.billing.providers.universal;

import ru.yandex.quasar.billing.beans.PricingOptionType;

enum PurchaseType {
    BUY(PricingOptionType.BUY),
    RENT(PricingOptionType.RENT),
    SUBSCRIPTION(PricingOptionType.SUBSCRIPTION);

    private final PricingOptionType pricingOptionType;

    PurchaseType(PricingOptionType pricingOptionType) {
        this.pricingOptionType = pricingOptionType;
    }

    public PricingOptionType getPricingOptionType() {
        return pricingOptionType;
    }
}

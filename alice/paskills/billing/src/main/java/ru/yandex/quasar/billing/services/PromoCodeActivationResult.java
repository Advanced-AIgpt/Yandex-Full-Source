package ru.yandex.quasar.billing.services;

import javax.annotation.Nullable;

import lombok.Data;

import ru.yandex.quasar.billing.beans.PricingOption;
import ru.yandex.quasar.billing.beans.PricingOptionType;

@Data
public class PromoCodeActivationResult {
    private final PricingOptionType type;
    @Nullable
    private final Integer subscriptionPeriodDays;
    @Nullable
    private final PricingOption subscriptionPricingOption;
    @Nullable
    private final Long subscriptionId;
}

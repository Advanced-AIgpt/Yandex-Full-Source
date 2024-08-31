package ru.yandex.quasar.billing.services;

import java.util.List;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import lombok.Data;

import ru.yandex.quasar.billing.beans.DeliveryInfo;
import ru.yandex.quasar.billing.beans.PricingOption;

@Data
public class SkillPurchaseOffer {
    @Nonnull
    private final String purchaseRequestId;
    @Nullable
    private final String imageUrl;
    @Nullable
    private final String description;
    @Nonnull
    private final String merchantKey;
    @Nonnull
    private final List<PricingOption> pricingOptions;
    @Nullable
    private final DeliveryInfo deliveryInfo;
    private final boolean testPayment;
}

package ru.yandex.quasar.billing.providers.amediateka.model;

import lombok.Data;

import ru.yandex.quasar.billing.beans.PricingOption;

/**
 * A payload we send in {@link PricingOption#getProviderPayload()} to later use in the billing process
 */
@Data
public class ProviderPayload {
    private final String priceUid;
    private final String bundleId;

}

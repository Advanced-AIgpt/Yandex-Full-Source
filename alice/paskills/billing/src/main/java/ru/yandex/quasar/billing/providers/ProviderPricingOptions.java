package ru.yandex.quasar.billing.providers;

import java.time.Instant;
import java.util.List;

import javax.annotation.Nullable;

import lombok.Data;

import ru.yandex.quasar.billing.beans.PricingOption;

/**
 * Information about the way content may be purchased from the provider
 * and if it's already available
 */
@Data
public class ProviderPricingOptions {
    // is content already available (purchased or by subscriptions for example) for the user by the provider
    private final boolean available;
    // temporary availability (RENT)
    @Nullable
    private final Instant availableUntil;
    // list of purchase options so that content become available
    private final List<PricingOption> pricingOptions;

    public static ProviderPricingOptions create(boolean available, List<PricingOption> pricingOptions,
                                                @Nullable Instant availableUntil) {
        return new ProviderPricingOptions(available, availableUntil, pricingOptions);
    }

}

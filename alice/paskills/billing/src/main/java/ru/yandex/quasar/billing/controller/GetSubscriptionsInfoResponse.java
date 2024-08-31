package ru.yandex.quasar.billing.controller;

import java.time.Instant;
import java.util.Collection;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonInclude;
import lombok.Builder;
import lombok.Data;

import ru.yandex.quasar.billing.beans.PricingOption;
import ru.yandex.quasar.billing.beans.ProviderContentItem;

@Data
class GetSubscriptionsInfoResponse {

    private final Collection<Item> items;

    @Data
    @Builder
    @JsonInclude(JsonInclude.Include.NON_NULL)
    static class Item {
        private final String provider;
        private final ProviderContentItem providerContentItem;
        private final String title;
        private final boolean cancellable;
        @Nullable
        private final Instant activeTill;
        @Nullable
        private final Long subscriptionId;

        @Nullable
        private final PricingOption pricingOption;

        @Nullable
        private final Instant nextPaymentDate;

        // based on the flag we understand how do we know active till date
        private final Boolean providerLoginRequired;

        public static ItemBuilder builder(String provider,
                                          ProviderContentItem providerContentItem,
                                          String title) {
            return new ItemBuilder()
                    .provider(provider)
                    .providerContentItem(providerContentItem)
                    .title(title);
        }
    }
}

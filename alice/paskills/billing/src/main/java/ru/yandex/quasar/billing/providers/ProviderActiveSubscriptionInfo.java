package ru.yandex.quasar.billing.providers;

import java.time.Instant;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import lombok.Builder;
import lombok.Data;

import ru.yandex.quasar.billing.beans.ProviderContentItem;

@Data
@Builder
public class ProviderActiveSubscriptionInfo {
    @Nonnull
    private final ProviderContentItem contentItem;
    @Nonnull
    private final String title;
    @Nullable
    private final String description;
    @Nullable
    private final String descriptionShort;
    private final Instant activeTill;
    /**
     * date of next payment if subscription is processed externally (mediabilling)
     */
    @Nullable
    private final Instant nextPaymentDate;

    public static ProviderActiveSubscriptionInfoBuilder builder(ProviderContentItem item) {
        return new ProviderActiveSubscriptionInfoBuilder()
                .contentItem(item);
    }
}

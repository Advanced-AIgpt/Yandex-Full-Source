package ru.yandex.quasar.billing.services.content;

import java.time.Instant;

import javax.annotation.Nullable;

import lombok.AccessLevel;
import lombok.Builder;
import lombok.Data;
import lombok.RequiredArgsConstructor;

import ru.yandex.quasar.billing.beans.ProviderContentItem;

@Data
@RequiredArgsConstructor(access = AccessLevel.PRIVATE)
@Builder
public class ActiveSubscriptionSummary {
    private final String title;
    private final ProviderContentItem providerContentItem;
    private final Instant activeTill;
    @Nullable
    private final Long subscriptionId;
    @Nullable
    private final Instant nextPaymentDate;
    private final boolean renewEnabled;
    private final boolean providerLoginRequired;

}

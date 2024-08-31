package ru.yandex.quasar.billing.services.promo;

import java.math.BigDecimal;
import java.time.Instant;
import java.util.Currency;

import javax.annotation.Nullable;

public sealed interface PromoInfo {
    record FreeDays(String productId, Integer promoDays) implements PromoInfo {
    }

    record Subscription(
            String productId,
            Integer promoDays,
            BigDecimal amount,
            Currency currency,
            @Nullable Instant firstPayment
    ) implements PromoInfo {
    }

    record SubscriptionPartly(String productId) implements PromoInfo {
    }

    record PlusPoints(BigDecimal amount) implements PromoInfo {
    }
}

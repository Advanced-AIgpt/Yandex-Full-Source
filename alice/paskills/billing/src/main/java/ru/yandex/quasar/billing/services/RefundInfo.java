package ru.yandex.quasar.billing.services;

import java.math.BigDecimal;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import lombok.Data;

@Data
public class RefundInfo {
    @Nonnull
    private final String purchaseToken;
    @Nonnull
    private final String refundId;
    @Nullable
    private final String subscriptionId;
    @Nullable
    private final String fiscalUrl;
    @Nonnull
    private final BigDecimal amount;
    @Nonnull
    private final String uid;
}

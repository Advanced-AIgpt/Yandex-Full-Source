package ru.yandex.quasar.billing.services;

import java.math.BigDecimal;
import java.time.Instant;

import javax.annotation.Nonnull;

import lombok.AccessLevel;
import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Data;

@Data
@AllArgsConstructor(access = AccessLevel.PRIVATE)
@Builder
public class TransactionInfo {
    private final String provider;
    private final String title;
    @Nonnull
    private final String purchaseToken;
    private final String paymentMethodId;
    @Nonnull
    private final String account;
    @Nonnull
    private final BigDecimal amount;
    @Nonnull
    private final String currency;
    @Nonnull
    private final Instant paymentDate;
    @Nonnull
    private final String cardType;
}

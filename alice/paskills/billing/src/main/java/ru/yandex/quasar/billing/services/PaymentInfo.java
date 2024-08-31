package ru.yandex.quasar.billing.services;

import java.math.BigDecimal;
import java.time.Instant;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import lombok.Data;

@Data
public class PaymentInfo {
    @Nonnull
    private final String purchaseToken;
    @Nullable
    private final String paymentMethodId;
    @Nullable
    private final String account;
    private final BigDecimal amount;
    private final String currency;
    private final Instant paymentDate;
    @Nullable
    private final String cardType;
    private final Status status;
    // fact clearing date
    @Nullable
    private final Instant clearDate;

    public enum Status {
        CLEARED,
        AUTHORIZED,
        ERROR_NOT_ENOUGH_FUNDS,
        ERROR_EXPIRED_CARD,
        ERROR_LIMIT_EXCEEDED,
        ERROR_TRY_LATER,
        ERROR_DO_NOT_TRY_LATER,
        ERROR_UNKNOWN,
        REFUND
    }
}

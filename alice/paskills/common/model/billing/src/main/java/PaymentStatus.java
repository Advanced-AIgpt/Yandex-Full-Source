package ru.yandex.alice.paskills.common.billing.model;

public enum PaymentStatus {
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

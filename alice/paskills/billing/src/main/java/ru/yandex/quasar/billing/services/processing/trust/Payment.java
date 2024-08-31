package ru.yandex.quasar.billing.services.processing.trust;

import lombok.Data;

@Data
public class Payment {
    private final Status status;

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

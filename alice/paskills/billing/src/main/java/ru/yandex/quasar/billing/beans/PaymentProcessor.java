package ru.yandex.quasar.billing.beans;

import com.fasterxml.jackson.annotation.JsonEnumDefaultValue;

/**
 * Known payment processors.
 * {@code TRUST} is used for native provider payments
 * {@code YANDEX_PAY} used for external skills payments
 * {@code FREE} used for free payments
 */
public enum PaymentProcessor {
    @JsonEnumDefaultValue
    TRUST,
    YANDEX_PAY,
    MEDIABILLING,
    FREE,
}

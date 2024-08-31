package ru.yandex.quasar.billing.services.processing.trust;

import com.fasterxml.jackson.annotation.JsonCreator;

public enum RefundStatus {
    success,
    error,
    failed,
    wait_for_notification,
    unknown;

    @JsonCreator
    static RefundStatus parse(String value) {
        try {
            return RefundStatus.valueOf(value);
        } catch (NullPointerException | IllegalArgumentException e) {
            return unknown;
        }
    }
}

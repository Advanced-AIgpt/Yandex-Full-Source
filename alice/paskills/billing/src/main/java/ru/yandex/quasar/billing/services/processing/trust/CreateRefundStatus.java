package ru.yandex.quasar.billing.services.processing.trust;

import com.fasterxml.jackson.annotation.JsonCreator;

public enum CreateRefundStatus {
    success,
    unknown;

    @JsonCreator
    static CreateRefundStatus parse(String value) {
        try {
            return CreateRefundStatus.valueOf(value);
        } catch (NullPointerException | IllegalArgumentException e) {
            return unknown;
        }
    }
}

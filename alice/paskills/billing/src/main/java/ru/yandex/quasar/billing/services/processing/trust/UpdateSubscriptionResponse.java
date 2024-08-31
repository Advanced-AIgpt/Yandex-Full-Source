package ru.yandex.quasar.billing.services.processing.trust;

import lombok.Data;

@Data
public class UpdateSubscriptionResponse {
    private static final UpdateSubscriptionResponse SUCCESS_INSTANCE = new UpdateSubscriptionResponse("success");
    private final String status;

    public static UpdateSubscriptionResponse success() {
        return SUCCESS_INSTANCE;
    }
}

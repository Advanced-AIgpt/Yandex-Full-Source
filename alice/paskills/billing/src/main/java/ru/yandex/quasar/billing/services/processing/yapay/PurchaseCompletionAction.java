package ru.yandex.quasar.billing.services.processing.yapay;

import com.fasterxml.jackson.annotation.JsonValue;

import ru.yandex.quasar.billing.util.HasCode;

/**
 * Action after the payment has been held.
 * https://github.yandex-team.ru/Billing/trust-frontend/tree/master/packages/trust-form#paymentcompletionaction
 */
public enum PurchaseCompletionAction implements HasCode<String> {
    REDIRECT_ALWAYS("redirect_always");

    private final String code;

    PurchaseCompletionAction(String code) {
        this.code = code;
    }

    @Override
    @JsonValue
    public String getCode() {
        return code;
    }
}

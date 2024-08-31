package ru.yandex.quasar.billing.controller;

import com.fasterxml.jackson.annotation.JsonInclude;
import lombok.Data;

@Data
class InitPurchaseProcessResponse {
    @JsonInclude(JsonInclude.Include.NON_NULL)
    private final String purchaseToken;
    @JsonInclude(JsonInclude.Include.NON_EMPTY)
    private final String redirectUrl;
}

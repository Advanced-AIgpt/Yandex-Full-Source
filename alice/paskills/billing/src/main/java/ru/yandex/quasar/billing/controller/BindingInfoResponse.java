package ru.yandex.quasar.billing.controller;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;

@Data
class BindingInfoResponse {
    private final String status;
    @JsonProperty("payment_method_id")
    private final String paymentMethodId;
}

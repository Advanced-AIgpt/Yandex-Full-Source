package ru.yandex.quasar.billing.controller;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;

@Data
class StartRefundResponse {
    @JsonProperty("refund_id")
    private final String refundId;
}

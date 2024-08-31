package ru.yandex.quasar.billing.controller;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;

@Data
class InitSkillPurchaseResponse {
    @JsonProperty("order_id")
    private final String orderId;
    @JsonProperty("url")
    private final String url;
}

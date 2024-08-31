package ru.yandex.quasar.billing.services.processing.trust;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;

@Data
public class CreateOrderRequest {
    @JsonProperty("product_id")
    private final String productId;
}

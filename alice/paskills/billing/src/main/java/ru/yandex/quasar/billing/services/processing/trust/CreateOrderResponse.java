package ru.yandex.quasar.billing.services.processing.trust;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;

@Data
public class CreateOrderResponse {
    private final String status;
    @JsonProperty("status_code")
    private final String statucCode;
    @JsonProperty("order_id")
    private final String orderId;
    @JsonProperty("product_id")
    private final String productId;
}

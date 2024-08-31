package ru.yandex.quasar.billing.services.processing.trust;

import java.util.List;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;

@Data
public class CreateOrderBatchResponse {
    private final String status;
    @JsonProperty("status_code")
    private final String statucCode;
    private final List<Order> orders;

    @Data
    public static class Order {
        @JsonProperty("order_id")
        private final String orderId;
    }
}

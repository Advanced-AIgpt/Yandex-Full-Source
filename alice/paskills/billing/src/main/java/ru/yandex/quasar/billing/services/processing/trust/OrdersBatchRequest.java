package ru.yandex.quasar.billing.services.processing.trust;

import java.util.List;
import java.util.stream.Collectors;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;

@Data
public class OrdersBatchRequest {
    private final List<OrderProduct> orders;

    private OrdersBatchRequest(List<OrderProduct> orders) {
        this.orders = orders;
    }

    public static OrdersBatchRequest create(List<String> productIds) {
        return new OrdersBatchRequest(productIds.stream().map(OrderProduct::new).collect(Collectors.toList()));
    }

    @Data
    private static class OrderProduct {
        @JsonProperty("product_id")
        private final String productId;

    }
}

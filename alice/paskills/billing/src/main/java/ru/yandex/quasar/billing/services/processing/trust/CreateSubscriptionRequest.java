package ru.yandex.quasar.billing.services.processing.trust;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;

@Data
class CreateSubscriptionRequest {
    @JsonProperty("product_id")
    private final String productId;
    @JsonProperty("order_id")
    private final String orderId;
    @JsonProperty("region_id")
    private final int regionId;
    @JsonProperty("commission_category")
    private final String commissionCategory;
}

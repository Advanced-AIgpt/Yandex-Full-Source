package ru.yandex.quasar.billing.providers.universal;

import java.util.List;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;

@Data
class PurchasedItems {
    @JsonProperty("purchased_products")
    private final List<PurchasedItem> purchasedProducts;
}

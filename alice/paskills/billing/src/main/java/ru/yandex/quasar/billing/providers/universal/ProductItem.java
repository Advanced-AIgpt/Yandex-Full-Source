package ru.yandex.quasar.billing.providers.universal;

import java.util.List;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Builder;
import lombok.Data;

@Data
@Builder()
class ProductItem {
    @JsonProperty("product_id")
    private final String productId;

    private final String title;
    @Nullable
    private final String description;
    @Nullable
    private final String descriptionShort;
    @JsonProperty("product_type")
    private final ProductType productType;

    @JsonProperty("is_content_item")
    private final boolean contentItem;

    private final List<ProductPrice> prices;

    @JsonProperty("image_url")
    @Nullable
    private final String imageUrl;

    public static ProductItemBuilder builder(String id, String title, ProductType type) {
        return new ProductItemBuilder()
                .productId(id)
                .title(title)
                .productType(type);
    }

}

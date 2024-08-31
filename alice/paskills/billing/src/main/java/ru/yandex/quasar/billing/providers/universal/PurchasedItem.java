package ru.yandex.quasar.billing.providers.universal;

import java.time.Instant;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonFormat;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.annotation.JsonSerialize;
import com.fasterxml.jackson.datatype.jsr310.ser.InstantSerializer;
import lombok.Builder;
import lombok.Data;

@Data
@Builder()
class PurchasedItem {
    @JsonProperty("product_id")
    private final String productId;

    private final String title;
    @Nullable
    private final String description;
    @Nullable
    private final String descriptionShort;
    @JsonProperty("purchased_at")
    @JsonFormat(shape = JsonFormat.Shape.STRING)
    @JsonSerialize(using = InstantSerializer.class)
    @Nullable
    private final Instant purchasedAt;
    @JsonProperty("active_till")
    @Nullable
    @JsonFormat(shape = JsonFormat.Shape.STRING)
    @JsonSerialize(using = InstantSerializer.class)
    private final Instant activeTill;

    private final boolean renewDisabled;

    @JsonProperty("is_content_item")
    private final boolean contentItem;
    @JsonProperty("item_type")
    private final ProductType itemType;
    @Nullable
    private final ProductPrice price;

    @Nullable
    @JsonProperty("next_payment_date")
    private final Instant nextPaymentDate;

    public static PurchasedItemBuilder builder(ProductItem productItem, ProductPrice price) {
        return new PurchasedItemBuilder()
                .productId(productItem.getProductId())
                .title(productItem.getTitle())
                .description(productItem.getDescription())
                .descriptionShort(productItem.getDescriptionShort())
                .itemType(productItem.getProductType())
                .contentItem(productItem.isContentItem())
                .price(price);
    }
}

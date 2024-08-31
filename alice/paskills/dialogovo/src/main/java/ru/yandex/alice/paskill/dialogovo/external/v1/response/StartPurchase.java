package ru.yandex.alice.paskill.dialogovo.external.v1.response;

import java.math.BigDecimal;
import java.util.Currency;
import java.util.List;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;
import javax.validation.Valid;
import javax.validation.constraints.NotEmpty;
import javax.validation.constraints.Positive;
import javax.validation.constraints.Size;

import com.fasterxml.jackson.annotation.JsonFormat;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.annotation.JsonValue;
import com.fasterxml.jackson.databind.node.ObjectNode;
import lombok.Data;

@Data
public class StartPurchase {
    @JsonProperty("purchase_request_id")
    @Size(max = 40)
    @Nonnull
    private final String purchaseRequestId;

    @JsonProperty("image_url")
    @Nullable
    private final String imageUrl;

    @Size(max = 512)
    private final String caption;

    @Size(max = 2048)
    private final String description;

    /**
     * Currency is based on ISO-4217 codes.
     */
    @Nonnull
    private final Currency currency;

    @JsonProperty("type")
    @Nonnull
    private final PricingOptionType type;

    @JsonProperty("payload")
    @Nonnull
    private final ObjectNode payload;

    @JsonProperty("merchant_key")
    @Size(max = 36)
    @Nonnull
    private final String merchantKey;

    @Valid
    @NotEmpty
    private final List<PurchaseRequestProduct> products;

    @JsonProperty("test_payment")
    private final boolean testPayment;

    /**
     * Delivery product with address
     */
    @Valid
    @Nullable
    private final PurchaseDeliveryInfo delivery;

    @Data
    static class PurchaseRequestProduct {
        /**
         * UUID of the product provided by skill
         */
        @JsonProperty("product_id")
        @NotEmpty
        @Nonnull
        private final String productId;

        @Size(max = 256)
        @NotEmpty
        @Nonnull
        private final String title;

        @JsonProperty("user_price")
        @JsonFormat(shape = JsonFormat.Shape.STRING)
        @Positive
        @Nonnull
        private final BigDecimal userPrice;

        @JsonFormat(shape = JsonFormat.Shape.STRING)
        @Positive
        @Nonnull
        private final BigDecimal price;

        @JsonFormat(shape = JsonFormat.Shape.STRING)
        @Positive
        @Nonnull
        private final BigDecimal quantity;

        @JsonProperty("nds_type")
        @Nonnull
        private final NdsType ndsType;
    }

    @Data
    static class PurchaseDeliveryInfo {
        @NotEmpty
        @Nonnull
        private final String city;

        @Nullable
        private final String settlement;

        @Nullable
        private final String index;

        @NotEmpty
        @Nonnull
        private final String street;

        @NotEmpty
        @Nonnull
        private final String house;

        @Nullable
        private final String housing;

        @Nullable
        private final String building;

        @Nullable
        private final String porch;

        @Nullable
        private final String floor;

        @Nullable
        private final String flat;

        @Nonnull
        private final BigDecimal price;

        @JsonProperty("nds_type")
        @Nonnull
        private final NdsType ndsType;
    }

    public enum NdsType {
        NDS_20("nds_20"),
        NDS_18("nds_18"),
        NDS_10("nds_10"),
        NDS_0("nds_0"),
        NDS_NONE("nds_none"),
        NDS_18_118("nds_18_118"),
        NDS_10_110("nds_10_110"),
        NDS_20_120("nds_20_120");

        private final String code;
        NdsType(String code) {
            this.code = code;
        }

        @JsonValue
        public String getCode() {
            return code;
        }
    }

    public enum PricingOptionType {
        BUY
    }
}

package ru.yandex.quasar.billing.providers.universal;

import java.math.BigDecimal;
import java.time.Instant;
import java.util.Currency;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonFormat;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.annotation.JsonRawValue;
import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import com.fasterxml.jackson.databind.annotation.JsonSerialize;
import com.fasterxml.jackson.datatype.jsr310.ser.InstantSerializer;
import lombok.Builder;
import lombok.Data;

import ru.yandex.quasar.billing.beans.PaymentProcessor;
import ru.yandex.quasar.billing.beans.PricingOption;
import ru.yandex.quasar.billing.controller.BillingController;

@Data
@Builder
class ProductPrice {
    @JsonProperty("price_option_id")
    @Nonnull
    private final String id;
    @JsonProperty("user_price")
    @Nonnull
    private final BigDecimal userPrice;
    @Nonnull
    private final BigDecimal price;
    @Nonnull
    private final String title;
    @JsonProperty("purchase_type")
    @Nonnull
    private final PurchaseType purchaseType;
    @Nonnull
    private final Currency currency;
    @JsonRawValue
    @JsonDeserialize(using = BillingController.AnythingToString.class)
    @JsonProperty("purchase_payload")
    private final String purchasePayload;
    private final Quality quality;
    private final String period;

    @Nullable
    @JsonFormat(shape = JsonFormat.Shape.STRING)
    @JsonSerialize(using = InstantSerializer.class)
    @JsonProperty("expired_at")
    private final Instant expiredAt;

    @Nullable
    @JsonFormat(shape = JsonFormat.Shape.STRING)
    @JsonSerialize(using = InstantSerializer.class)
    @JsonProperty("purchasable_from")
    private final Instant purchasableFrom;

    @JsonProperty("trial_period")
    @Nullable
    private final String trialPeriod;

    @JsonProperty("first_payment_period")
    @Nullable
    private final String firstPaymentPeriod;

    @Nullable
    private final PaymentProcessor processing;

    @Nullable
    @JsonProperty("discount_type")
    private final DiscountType discountType;

    public static ProductPriceBuilder builder(String id, BigDecimal userPrice, PurchaseType purchaseType) {
        return new ProductPriceBuilder()
                .id(id)
                .userPrice(userPrice)
                .price(userPrice)
                .purchaseType(purchaseType);
    }

    public enum DiscountType {
        FIRST_PAYMENT(PricingOption.DiscountType.FIRST_PAYMENT);


        private final PricingOption.DiscountType discountType;

        DiscountType(PricingOption.DiscountType discountType) {
            this.discountType = discountType;
        }

        public PricingOption.DiscountType getDiscountType() {
            return discountType;
        }
    }

}

package ru.yandex.quasar.billing.beans;

import java.math.BigDecimal;
import java.util.Collections;
import java.util.List;
import java.util.Objects;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonGetter;
import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import com.fasterxml.jackson.databind.annotation.JsonSerialize;
import com.fasterxml.jackson.databind.ser.std.ToStringSerializer;
import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Data;

import ru.yandex.quasar.billing.services.processing.NdsType;

import static lombok.AccessLevel.PRIVATE;

@Data
@AllArgsConstructor(access = PRIVATE)
@Builder
public class PricingOption {

    private final BigDecimal userPrice;
    private final BigDecimal price;
    // ISO 4217 code such as  "RUB"
    private final String currency;
    private final String providerPayload;
    // either item's title is not null or the parent one
    private final String title;
    private final PricingOptionType type;
    @Nullable
    private final ContentQuality quality;
    @Nullable
    @Deprecated
    // null for external skills
    // should be out of Pricing options, should be passed to method explicitly
    private final String provider;

    // user SubscriptionPeriod instead
    @Deprecated
    @Nullable
    private final Integer subscriptionPeriodDays;

    private final boolean specialCommission;

    // the item user is going to buy
    @Nullable
    private final ProviderContentItem purchasingItem;

    @Nullable
    private final String optionId;
    // subitems for complex purchases
    @Nullable
    private final List<PricingOptionLine> items;

    @Nullable
    private final PaymentProcessor processor;

    @Nullable
    @JsonDeserialize(using = LogicalPeriodDeserializer.class)
    @JsonSerialize(using = ToStringSerializer.class)
    private final LogicalPeriod subscriptionPeriod;
    @Nullable
    @JsonDeserialize(using = LogicalPeriodDeserializer.class)
    @JsonSerialize(using = ToStringSerializer.class)
    private final LogicalPeriod subscriptionTrialPeriod;

    @Nullable
    private final DiscountType discountType;

    @Nullable
    private final String imageUrl;

    public static PricingOptionBuilder builder(String title, PricingOptionType type, BigDecimal userPrice,
                                               BigDecimal price, String currency) {
        return new PricingOptionBuilder()
                .title(title)
                .type(type)
                .userPrice(userPrice)
                .price(price)
                .currency(currency);
    }

    public static PricingOption createForPurchaseOffer(String purchaseRequestId, PricingOptionType type,
                                                       List<PricingOptionLine> pricingOptionItems, String currency,
                                                       String providerPayload, String caption) {
        BigDecimal totalPrice = pricingOptionItems.stream()
                .map(it -> it.getPrice().multiply(it.getQuantity()))
                .reduce(BigDecimal.ZERO, BigDecimal::add);
        BigDecimal totalUserPrice = pricingOptionItems.stream()
                .map(it -> it.getUserPrice().multiply(it.getQuantity()))
                .reduce(BigDecimal.ZERO, BigDecimal::add);

        return new PricingOptionBuilder()
                // as skill purchase offers contain only a single option, put purchaseRequestId as the option ID
                .title(caption)
                .optionId(purchaseRequestId)
                .type(type)
                .userPrice(totalUserPrice)
                .price(totalPrice)
                .currency(currency)
                .providerPayload(providerPayload)
                .items(pricingOptionItems)
                .build();
    }

    @Nonnull
    public List<PricingOptionLine> getItems() {
        if (items == null) {
            return Collections.singletonList(new PricingOptionLine(optionId, userPrice, price, title, BigDecimal.ONE,
                    NdsType.nds_20));
        } else {
            return items;
        }
    }

    @Nullable
    @JsonGetter("subscriptionPeriod")
    public LogicalPeriod getSubscriptionPeriod() {
        // convert old daily periods to Period
        return subscriptionPeriod == null && subscriptionPeriodDays != null ?
                LogicalPeriod.ofDays(subscriptionPeriodDays) : subscriptionPeriod;
    }

    @Nullable
    @Deprecated
    @JsonGetter("subscriptionPeriodDays")
    private Integer getSubscriptionPeriodDays() {
        return subscriptionPeriodDays == null && subscriptionPeriod != null ? (Integer) subscriptionPeriod.getDays()
                : subscriptionPeriodDays;
    }

    public boolean equalsForBilling(PricingOption that) {
        return that != null
                && userPrice.compareTo(that.userPrice) == 0
                && Objects.equals(currency, that.currency)
                && type == that.type
                && quality == that.quality
                && Objects.equals(getSubscriptionPeriod(), that.getSubscriptionPeriod())
                && Objects.equals(purchasingItem, that.purchasingItem);
    }

    public enum DiscountType {
        FIRST_PAYMENT
    }

    @Data
    public static class PricingOptionLine {
        @Nullable
        private final String productId;
        @Nonnull
        private final BigDecimal userPrice;
        @Nonnull
        private final BigDecimal price;
        @Nonnull
        private final String title;
        @Nonnull
        private final BigDecimal quantity;
        @Nullable
        private final NdsType ndsType;

        @Nonnull
        public NdsType getNdsType() {
            return ndsType != null ? ndsType : NdsType.nds_20;
        }
    }

}

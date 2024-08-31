package ru.yandex.quasar.billing.controller;

import java.math.BigDecimal;
import java.util.List;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;
import javax.validation.constraints.NotEmpty;

import com.fasterxml.jackson.annotation.JsonGetter;
import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Builder;
import lombok.Data;

import ru.yandex.quasar.billing.dao.PurchaseOfferStatus;
import ru.yandex.quasar.billing.services.processing.NdsType;

@Data
@Builder
class PurchaseOfferData {

    /**
     * UUID of the purchase offer
     */
    @JsonProperty("purchaseOfferUuid")
    @Nonnull
    private final String purchaseOfferUuid;

    /**
     * external purchase request ID generated by the caller (skill) - UUID
     */
    @Nonnull
    private final String purchaseRequestId;

    @Nullable // offer may spread across multiple providers
    private final String skillUuid;

    // skill name
    @Nonnull
    private final String name;
    @Nullable
    private final String imageUrl;
    @Nonnull
    private final String selectedOptionUuid;

    @Nonnull
    private final MerchantInfo merchantInfo;


    //ISO 4217 code such as  "RUB"
    @Nonnull
    private final String currency;
    @Nonnull
    private final PurchaseOfferStatus status;

    @Nullable
    private final PurchaseDeliveryInfo deliveryInfo;
    @Nullable
    private final PaymentMethodInfo paymentMethodInfo;
    @Nonnull
    private List<PurchaseOfferProductData> products;

    public static PurchaseOfferDataBuilder builder(String purchaseOfferUuid, String purchaseRequestId,
                                                   String selectedOptionUuid) {
        return new PurchaseOfferDataBuilder()
                .purchaseOfferUuid(purchaseOfferUuid)
                .purchaseRequestId(purchaseRequestId)
                .selectedOptionUuid(selectedOptionUuid);
    }

    @JsonGetter(value = "totalUserPrice")
    BigDecimal getTotalUserPrice() {
        return products.stream()
                .map(PurchaseOfferProductData::getTotalUserPrice)
                .reduce(BigDecimal.ZERO, BigDecimal::add)
                .add(deliveryInfo != null ? deliveryInfo.getPrice() : BigDecimal.ZERO);
    }

    @JsonGetter(value = "totalPrice")
    BigDecimal getTotalPrice() {
        return products.stream()
                .map(PurchaseOfferProductData::getTotalPrice)
                .reduce(BigDecimal.ZERO, BigDecimal::add)
                .add(deliveryInfo != null ? deliveryInfo.getPrice() : BigDecimal.ZERO);
    }

    @Data
    @Builder
    static class PurchaseOfferProductData {
        @Nonnull
        private final String productId;
        @Nonnull
        private final BigDecimal quantity;
        @Nonnull
        private final String title;
        @Nonnull
        private final BigDecimal userPrice;
        private final BigDecimal price;
        private final NdsType ndsType;

        @JsonProperty("totalUserPrice")
        BigDecimal getTotalUserPrice() {
            return quantity.multiply(userPrice);
        }

        @JsonProperty("totalPrice")
        BigDecimal getTotalPrice() {
            return quantity.multiply(price);
        }
    }

    @Data
    @Builder
    static class PurchaseDeliveryInfo {
        @Nonnull
        private final String productId;
        @Nullable
        private final String index;
        @NotEmpty
        private final String city;
        @Nullable
        private final String settlement;
        @NotEmpty
        private final String street;
        @NotEmpty
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
    }

    @Data
    @Builder
    static class MerchantInfo {
        @Nonnull
        private final String name;
        @Nullable
        private final String address;
        @Nullable
        private final String ogrn;
        @Nullable
        private final String inn;
        @Nullable
        private final String workingHours;
        @Nullable
        private final String description;
    }

    @Data
    static class PaymentMethodInfo {
        @Nonnull
        @JsonProperty("id")
        private final String paymentMethodId;
        @Nonnull
        private final String account;
        @Nonnull
        private final String system;
    }
}

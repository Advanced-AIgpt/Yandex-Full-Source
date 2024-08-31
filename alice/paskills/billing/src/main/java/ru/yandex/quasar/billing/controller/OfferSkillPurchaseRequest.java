package ru.yandex.quasar.billing.controller;

import java.math.BigDecimal;
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;
import javax.validation.Valid;
import javax.validation.constraints.NotEmpty;
import javax.validation.constraints.NotNull;
import javax.validation.constraints.Positive;
import javax.validation.constraints.Size;

import com.fasterxml.jackson.annotation.JsonFormat;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.node.ObjectNode;
import lombok.Builder;
import lombok.Data;

import ru.yandex.quasar.billing.beans.DeliveryInfo;
import ru.yandex.quasar.billing.beans.PricingOption;
import ru.yandex.quasar.billing.beans.PricingOption.PricingOptionLine;
import ru.yandex.quasar.billing.beans.PricingOptionType;
import ru.yandex.quasar.billing.services.BillingService;
import ru.yandex.quasar.billing.services.SkillPurchaseOffer;
import ru.yandex.quasar.billing.services.processing.NdsType;
import ru.yandex.quasar.billing.services.processing.TrustCurrency;

@Data
class OfferSkillPurchaseRequest {
    @JsonProperty("skill")
    @NotNull
    private final SkillInfo skillInfo;
    @JsonProperty("session_id")
    @NotEmpty
    private final String sessionId;
    @JsonProperty("user_id")
    @NotEmpty
    private final String userId;

    @Nullable
    @JsonProperty("device_id")
    private final String deviceId;

    @Valid
    @JsonProperty("purchase_request")
    private final PurchaseRequest purchaseRequest;

    @Nullable
    @JsonProperty("webhook_request")
    private final ObjectNode webhookRequest;

    @Data
    static class PurchaseRequest {
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

        @NotNull
        private final TrustCurrency currency;

        @JsonProperty("type")
        @NotNull
        private final PricingOptionType type;

        @JsonProperty("payload")
        @Nonnull
        private final ObjectNode payload;

        @Nonnull
        @Size(max = 36)
        @JsonProperty("merchant_key")
        private final String merchantKey;

        // perhaps rename to options if support of multiple pricing options is required
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

        SkillPurchaseOffer toSkillPurchaseOffer() {

            List<PricingOptionLine> pricingOptionItems = products.stream()
                    .map(PurchaseRequestProduct::toPricingOptionItem)
                    .collect(Collectors.toCollection(ArrayList::new));

            DeliveryInfo deliveryInfo;
            if (delivery != null) {
                String deliveryProductId = UUID.randomUUID().toString();
                deliveryInfo = DeliveryInfo.builder()
                        .productId(deliveryProductId)
                        .city(delivery.getCity())
                        .settlement(delivery.getSettlement())
                        .index(delivery.getIndex())
                        .street(delivery.getStreet())
                        .house(delivery.getHouse())
                        .housing(delivery.getHousing())
                        .building(delivery.getBuilding())
                        .porch(delivery.getPorch())
                        .floor(delivery.getFloor())
                        .flat(delivery.getFlat())
                        .build();

                PricingOptionLine deliveryLine =
                        new PricingOptionLine(
                                deliveryProductId,
                                delivery.getPrice(),
                                delivery.getPrice(),
                                BillingService.DELIVERY_PRODUCT_NAME,
                                BigDecimal.ONE,
                                delivery.getNdsType()
                        );
                pricingOptionItems.add(deliveryLine);
            } else {
                deliveryInfo = null;
            }

            PricingOption pricingOption = PricingOption.createForPurchaseOffer(
                    purchaseRequestId,
                    type,
                    pricingOptionItems,
                    currency.getCurrencyCode(),
                    payload.toString(),
                    caption);

            return new SkillPurchaseOffer(
                    purchaseRequestId,
                    imageUrl,
                    description,
                    merchantKey,
                    List.of(pricingOption),
                    deliveryInfo,
                    testPayment
            );
        }
    }

    @Data
    static class PurchaseRequestProduct {
        // UUID of the product provided by provider
        @JsonProperty("product_id")
        @NotEmpty
        private final String productId;

        @NotEmpty
        @Size(max = 256)
        private final String title;

        @JsonProperty("user_price")
        @JsonFormat(shape = JsonFormat.Shape.STRING)
        @Positive
        @NotNull
        private final BigDecimal userPrice;

        @JsonFormat(shape = JsonFormat.Shape.STRING)
        @Positive
        @NotNull
        private final BigDecimal price;
        @JsonFormat(shape = JsonFormat.Shape.STRING)
        @Positive
        @NotNull
        private final BigDecimal quantity;

        @JsonProperty("nds_type")
        @NotNull
        private final NdsType ndsType;

        private PricingOptionLine toPricingOptionItem() {
            return new PricingOptionLine(productId, userPrice, price, title, quantity, ndsType);
        }

    }

    @Data
    static class SkillInfo {
        @JsonProperty("id")
        @NotEmpty
        private final String skillUuid;
        @NotEmpty
        @Size(max = 2048)
        private final String name;
        @JsonProperty("image_url")
        @Size(max = 4096)
        private final String imageUrl;
        @JsonProperty("callback_url")
        private final String callbackUrl;
    }

    @Data
    @Builder
    static class PurchaseDeliveryInfo {
        @NotEmpty
        private final String city;
        @Nullable
        private final String settlement;
        @Nullable
        private final String index;
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
        @NotNull
        private final BigDecimal price;
        @JsonProperty("nds_type")
        @NotNull
        private final NdsType ndsType;
    }
}

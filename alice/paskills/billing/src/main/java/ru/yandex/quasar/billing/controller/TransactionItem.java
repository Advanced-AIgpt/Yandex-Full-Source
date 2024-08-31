package ru.yandex.quasar.billing.controller;

import java.math.BigDecimal;
import java.time.Instant;
import java.util.List;
import java.util.function.Function;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonFormat;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.annotation.JsonValue;
import lombok.Builder;
import lombok.Data;

import ru.yandex.quasar.billing.beans.PricingOptionType;
import ru.yandex.quasar.billing.dao.PurchaseInfo;
import ru.yandex.quasar.billing.services.TransactionInfoV2;
import ru.yandex.quasar.billing.services.processing.NdsType;
import ru.yandex.quasar.billing.util.HasCode;

@Data
@Builder
public class TransactionItem {
    @JsonProperty("purchase_id")
    private final long purchaseId;

    @JsonProperty("purchase_type")
    @Nullable
    private final String purchaseType;

    @JsonProperty("skill_info")
    @Nullable
    private final SkillInfo skillInfo;

    @JsonProperty("masked_card_number")
    private final String maskedCardNumber;

    @JsonProperty("payment_system")
    private final String paymentSystem;

    @JsonProperty("total_price")
    private final BigDecimal totalPrice;

    private final String currency;

    private final Status status;

    @JsonProperty("status_change_date")
    @JsonFormat(shape = JsonFormat.Shape.STRING)
    private final Instant statusChangeDate;

    @JsonProperty("pricing_type")
    private final PricingType pricingType;

    private final List<Basket> basket;

    public static Function<TransactionInfoV2, TransactionItem> getTransactionItem() {
        return t -> TransactionItem.builder()
                .purchaseId(t.getPurchaseId())
                .purchaseType(t.getPurchaseType())
                .skillInfo(
                        t.getSkillInfo() != null ?
                                SkillInfo.builder()
                                        .name(t.getSkillInfo().getName())
                                        .slug(t.getSkillInfo().getSlug())
                                        .logoUrl(t.getSkillInfo().getLogoUrl())
                                        .merchantName(t.getSkillInfo().getMerchantName())
                                        .build() :
                                null
                )
                .maskedCardNumber(t.getMaskedCardNumber())
                .paymentSystem(t.getPaymentSystem())
                .totalPrice(t.getTotalPrice())
                .currency(t.getCurrency())
                .status(Status.from(t.getStatus()))
                .statusChangeDate(t.getStatusChangeDate())
                .pricingType(PricingType.from(t.getPricingType()))
                .basket(
                        t.getBasket().stream()
                                .map(basket -> Basket.builder()
                                        .userPrice(basket.getUserPrice())
                                        .price(basket.getPrice())
                                        .title(basket.getTitle())
                                        .quantity(basket.getQuantity())
                                        .ndsType(basket.getNdsType())
                                        .build())
                                .collect(Collectors.toList())
                )
                .build();
    }

    @Data
    @Builder
    static class SkillInfo {
        private final String slug;

        private final String name;

        @JsonProperty("merchant_name")
        private final String merchantName;

        @JsonProperty("logo_url")
        private final String logoUrl;
    }

    @Data
    @Builder
    static class Basket {
        @JsonProperty("user_price")
        private final BigDecimal userPrice;

        private final BigDecimal price;

        private final String title;

        private final BigDecimal quantity;

        @JsonProperty("nds_type")
        private final NdsType ndsType;
    }

    enum PricingType implements HasCode<String> {
        SUBSCRIPTION("subscription"),
        RENT("rent"),
        BUY("buy");

        private final String code;

        PricingType(String code) {
            this.code = code;
        }

        public static PricingType from(PricingOptionType pricingType) {
            switch (pricingType) {
                case SUBSCRIPTION:
                    return PricingType.SUBSCRIPTION;
                case RENT:
                    return PricingType.RENT;
                case BUY:
                    return PricingType.BUY;
                default:
                    throw new IllegalStateException("can't find PricingType for " + pricingType);
            }
        }

        @JsonValue
        @Override
        public String getCode() {
            return code;
        }
    }

    enum Status implements HasCode<String> {
        WAITING_FOR_PURCHASE("waiting_for_purchase"),
        WAITING_FOR_MERCHANT_CONFIRMATION("waiting_for_merchant_confirmation"),
        SUCCESS("success"),
        CANCELED("canceled"),
        ERROR("error");

        private final String code;

        Status(String code) {
            this.code = code;
        }

        public static Status from(PurchaseInfo.Status status) {
            switch (status) {
                case STARTED:
                case AUTHORIZED:
                case PROCESSED:
                    return Status.WAITING_FOR_PURCHASE;

                case WAITING_FOR_CLEARING:
                    return Status.WAITING_FOR_MERCHANT_CONFIRMATION;


                case CLEARED:
                case ALREADY_AVAILABLE:
                    return Status.SUCCESS;


                case REFUNDED:
                case NOT_CLEARED:
                    return Status.CANCELED;


                case ERROR_PAYMENT_OPTION_OBSOLETE:
                case ERROR_UNKNOWN:
                case ERROR_TRY_LATER:
                case ERROR_DO_NOT_TRY_LATER:
                case ERROR_NOT_ENOUGH_FUNDS:
                case ERROR_EXPIRED_CARD:
                case ERROR_LIMIT_EXCEEDED:
                case ERROR_NO_PROVIDER_ACC:
                    return Status.ERROR;

                default:
                    throw new IllegalStateException("can't find purchase Status for " + status);
            }
        }

        @Override
        @JsonValue
        public String getCode() {
            return code;
        }
    }
}

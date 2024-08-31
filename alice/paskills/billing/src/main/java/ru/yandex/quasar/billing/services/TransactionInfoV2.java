package ru.yandex.quasar.billing.services;

import java.math.BigDecimal;
import java.time.Instant;
import java.util.List;

import javax.annotation.Nullable;

import lombok.Builder;
import lombok.Data;

import ru.yandex.quasar.billing.beans.PricingOptionType;
import ru.yandex.quasar.billing.dao.PurchaseInfo;
import ru.yandex.quasar.billing.services.processing.NdsType;

@Data
@Builder
public class TransactionInfoV2 {
    private final long purchaseId;

    @Nullable
    private final String purchaseType;

    @Nullable
    private final SkillInfo skillInfo;

    @Nullable
    private final String maskedCardNumber;

    @Nullable
    private final String paymentSystem;

    private final BigDecimal totalPrice;

    @Nullable
    private final String currency;

    private final PurchaseInfo.Status status;

    @Nullable
    private final Instant statusChangeDate;

    private final PricingOptionType pricingType;

    private final List<Basket> basket;

    @Data
    @Builder
    public static class SkillInfo {
        private final String slug;

        private final String name;

        @Nullable
        private final String merchantName;

        private final String logoUrl;
    }

    @Data
    @Builder
    public static class Basket {
        private final BigDecimal userPrice;

        private final BigDecimal price;

        private final String title;

        private final BigDecimal quantity;

        private final NdsType ndsType;
    }
}

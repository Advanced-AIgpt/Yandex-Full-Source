package ru.yandex.quasar.billing.dao;

import java.math.BigDecimal;
import java.sql.Timestamp;
import java.time.Instant;
import java.time.temporal.ChronoUnit;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import lombok.Getter;
import lombok.Setter;

import ru.yandex.quasar.billing.beans.PaymentProcessor;
import ru.yandex.quasar.billing.beans.PricingOption;
import ru.yandex.quasar.billing.beans.ProviderContentItem;

@Getter
@Setter
public class PurchaseInfo {

    private Long purchaseId;
    private Long uid;
    private String purchaseToken;
    private Instant purchaseDate;
    @Nullable
    private ProviderContentItem contentItem;
    private PricingOption selectedOption;
    private Status status;
    private Timestamp callbackDate;
    @Nullable
    private Long subscriptionId;
    private Integer retriesCount;
    @Nullable
    private String securityToken;
    @Nullable // for backwards compatibility
    private String provider;
    @Nullable
    private Long skillInfoId;
    @Nullable
    private Long userSkillProductId;
    @Nullable
    private BigDecimal userPrice;
    @Nullable
    private BigDecimal originalPrice;
    // ISO 4217 code such as  "RUB"
    @Nullable
    private String currencyCode;
    @Nullable
    private Long partnerId;
    @Nullable
    private Long purchaseOfferId;
    private PaymentProcessor paymentProcessor;
    @Nullable
    private Instant refundDate;
    @Nullable
    private Long merchantId;
    @Nullable
    private String merchantName;
    @Nullable
    private Instant clearDate;

    /**
     * for Spring-JDBC mapper use onply
     */
    @Deprecated
    PurchaseInfo() {
    }

    @SuppressWarnings("ParameterNumber")
    private PurchaseInfo(
            Long purchaseId,
            Long uid,
            String purchaseToken,
            Instant purchaseDate,
            @Nullable String provider,
            @Nullable ProviderContentItem contentItem,
            PricingOption selectedOption,
            Status status,
            @Nullable Long subscriptionId,
            @Nullable String securityToken,
            BigDecimal userPrice,
            BigDecimal originalPrice,
            String currencyCode,
            @Nullable Long partnerId,
            @Nullable Long purchaseOfferId,
            @Nullable Long userSkillProductId,
            @Nullable Long skillInfoId,
            PaymentProcessor paymentProcessor,
            @Nullable Instant refundDate,
            @Nullable Long merchantId,
            @Nullable String merchantName
    ) {
        this.purchaseId = purchaseId;
        this.uid = uid;
        this.purchaseToken = purchaseToken;
        this.purchaseDate = purchaseDate;
        this.contentItem = contentItem;
        this.selectedOption = selectedOption;
        this.provider = provider;
        this.status = status;
        this.subscriptionId = subscriptionId;
        this.retriesCount = 0;
        this.securityToken = securityToken;
        this.userPrice = userPrice;
        this.originalPrice = originalPrice;
        this.currencyCode = currencyCode;
        this.partnerId = partnerId;
        this.purchaseOfferId = purchaseOfferId;
        this.userSkillProductId = userSkillProductId;
        this.paymentProcessor = paymentProcessor;
        this.skillInfoId = skillInfoId;
        this.refundDate = refundDate;
        this.merchantId = merchantId;
        this.merchantName = merchantName;
    }

    @SuppressWarnings("ParameterNumber")
    public static PurchaseInfo createSinglePayment(
            Long purchaseId,
            Long uid,
            String purchaseToken,
            @Nullable ProviderContentItem contentItem,
            PricingOption selectedOption,
            Status status,
            @Nullable String securityToken,
            @Nullable Long partnerId,
            @Nullable Long purchaseOfferId,
            @Nullable String provider,
            @Nullable Long skillInfoId,
            PaymentProcessor paymentProcessor,
            @Nullable Long submerchantId,
            @Nullable String merchantName) {
        return new PurchaseInfo(purchaseId,
                uid,
                purchaseToken,
                Instant.now().truncatedTo(ChronoUnit.MILLIS),
                provider,
                contentItem,
                selectedOption,
                status,
                null,
                securityToken,
                selectedOption.getUserPrice(),
                selectedOption.getPrice(),
                selectedOption.getCurrency(),
                partnerId,
                purchaseOfferId,
                null,
                skillInfoId,
                paymentProcessor,
                null,
                submerchantId,
                merchantName);
    }

    @SuppressWarnings("ParameterNumber")
    public static PurchaseInfo createFreeSkillProductPayment(
            Long purchaseId,
            Long uid,
            String purchaseToken,
            PricingOption selectedOption,
            Status status,
            Long userSkillProductId,
            String provider,
            Long skillInfoId) {
        return new PurchaseInfo(purchaseId,
                uid,
                purchaseToken,
                Instant.now(),
                provider,
                null,
                selectedOption,
                status,
                null,
                null,
                selectedOption.getUserPrice(),
                selectedOption.getPrice(),
                selectedOption.getCurrency(),
                null,
                null,
                userSkillProductId,
                skillInfoId,
                PaymentProcessor.FREE,
                null,
                null,
                null);
    }

    @SuppressWarnings("ParameterNumber")
    public static PurchaseInfo createSubscriptionPayment(
            Long purchaseId, Long uid, String purchaseToken,
            ProviderContentItem contentItem,
            PricingOption selectedOption, Status status,
            Long subscriptionId, boolean trial, Long partnerId,
            @Nullable String provider, PaymentProcessor paymentProcessor
    ) {
        return new PurchaseInfo(purchaseId,
                uid,
                purchaseToken,
                Instant.now(),
                provider,
                contentItem,
                selectedOption,
                status,
                subscriptionId,
                null,
                trial ? BigDecimal.ZERO : selectedOption.getUserPrice(),
                trial ? BigDecimal.ZERO : selectedOption.getPrice(),
                selectedOption.getCurrency(),
                partnerId,
                // purchaseOfferId is set on subscription
                null,
                null,
                null,
                paymentProcessor,
                null,
                null,
                null);
    }

    @Nonnull
    public PaymentProcessor getPaymentProcessor() {
        return paymentProcessor != null ? paymentProcessor : PaymentProcessor.TRUST;
    }

    public enum Status {
        STARTED(false),
        AUTHORIZED(false),
        PROCESSED(false),
        WAITING_FOR_CLEARING(false),
        CLEARED(false),
        NOT_CLEARED(false),
        ALREADY_AVAILABLE(false),
        ERROR_PAYMENT_OPTION_OBSOLETE(true),
        ERROR_UNKNOWN(true),
        ERROR_TRY_LATER(true),
        ERROR_DO_NOT_TRY_LATER(true),
        ERROR_NOT_ENOUGH_FUNDS(false),
        ERROR_EXPIRED_CARD(false),
        ERROR_LIMIT_EXCEEDED(false),
        REFUNDED(false),
        ERROR_NO_PROVIDER_ACC(true);

        private final boolean error;

        Status(boolean error) {
            this.error = error;
        }

        public boolean isError() {
            return error;
        }

    }

}

package ru.yandex.quasar.billing.dao;

import java.math.BigDecimal;
import java.sql.Timestamp;

import javax.annotation.Nullable;

import lombok.AccessLevel;
import lombok.AllArgsConstructor;
import lombok.Getter;
import lombok.Setter;
import lombok.ToString;

import ru.yandex.quasar.billing.beans.PaymentProcessor;
import ru.yandex.quasar.billing.beans.PricingOption;
import ru.yandex.quasar.billing.beans.ProviderContentItem;

@Setter
@Getter
@ToString
@AllArgsConstructor(access = AccessLevel.PRIVATE)
public class SubscriptionInfo {

    private Long subscriptionId;
    private Long uid;
    private Timestamp subscriptionDate;
    private ProviderContentItem contentItem;
    private PricingOption selectedOption;
    private Status status;
    private Timestamp activeTill;
    @Nullable // for external skill purchases
    private String provider;
    @Nullable
    private String securityToken;
    @Nullable
    private BigDecimal userPrice;
    @Nullable
    private BigDecimal originalPrice;
    // Subscription renewal period in ISO-8601 format
    @Nullable
    private String subscriptionPeriod;
    // Trial period in ISO-8601 format
    @Nullable
    private String trialPeriod;
    @Nullable
    private String productCode;
    // ISO 4217 code such as  "RUB"
    @Nullable
    private String currencyCode;
    // partner ID from TRUST
    private Long partnerId;
    // provider's subscription content item that was purchased.
    @Nullable
    private ProviderContentItem purchasedContentItem;
    private PaymentProcessor paymentProcessor;

    // for DAO
    @Deprecated
    SubscriptionInfo() {
    }

    //TODO: add purchaseOfferId when subscriptions become supported

    @SuppressWarnings("ParameterNumber")
    public static SubscriptionInfo create(
            Long subscriptionId,
            Long uid,
            Timestamp subscriptionDate,
            ProviderContentItem requestedContentItem,
            PricingOption selectedOption,
            Status status,
            Timestamp activeTill,
            @Nullable String securityToken,
            int trialPeriodDays,
            String productCode,
            Long partnerId,
            @Nullable String provider,
            @Nullable ProviderContentItem purchasingItem,
            PaymentProcessor paymentProcessor
    ) {
        return new SubscriptionInfo(
                subscriptionId,
                uid,
                subscriptionDate,
                requestedContentItem,
                selectedOption,
                status,
                activeTill,
                provider,
                securityToken,
                selectedOption.getUserPrice(),
                selectedOption.getPrice(),
                //according to ISO-8601 period is of form "P30D" so simply trunc leading "P" for TRUST
                selectedOption.getSubscriptionPeriod() != null ?
                        selectedOption.getSubscriptionPeriod().toString().substring(1) : null,
                trialPeriodDays > 0 ? "" + trialPeriodDays + "D" : null,
                productCode,
                selectedOption.getCurrency(),
                partnerId,
                purchasingItem,
                paymentProcessor);
    }

    public PaymentProcessor getPaymentProcessor() {
        return paymentProcessor != null ? paymentProcessor : PaymentProcessor.TRUST;
    }

    @Nullable
    public ProviderContentItem getPurchasedContentItem() {
        return purchasedContentItem != null ? purchasedContentItem : selectedOption.getPurchasingItem();
    }

    public enum Status {
        CREATED,
        ACTIVE,
        DISMISSED,
        EXPIRED // if subscription was ACTIVE and failed to renew
    }
}

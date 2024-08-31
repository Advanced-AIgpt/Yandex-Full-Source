package ru.yandex.quasar.billing.beans;

import java.math.BigDecimal;

import ru.yandex.quasar.billing.services.processing.TrustCurrency;

public interface PricingOptionTestUtil {
    default PricingOption createPricingOption(String provider, BigDecimal price, ProviderContentItem purchasingItem) {
        return PricingOption.builder("title", PricingOptionType.BUY, price, price, "RUB")
                .provider(provider)
                .purchasingItem(purchasingItem)
                .specialCommission(false)
                .build();

    }

    default PricingOption createPricingOption(String provider, BigDecimal price, PricingOptionType type,
                                              ProviderContentItem purchasingItem) {
        return createPricingOption(provider, price, price, type, purchasingItem);
    }

    default PricingOption createPricingOption(String provider, BigDecimal userPrice, BigDecimal price,
                                              PricingOptionType type, ProviderContentItem purchasingItem) {
        return PricingOption.builder("title", type, userPrice, price, "RUB")
                .provider(provider)
                .purchasingItem(purchasingItem)
                .specialCommission(false)
                .build();
    }

    default PricingOption createSkillPricingOption(String optionId, BigDecimal userPrice, BigDecimal price) {
        return PricingOption.builder("product_title", PricingOptionType.BUY, userPrice, price,
                TrustCurrency.RUB.getCurrencyCode())
                //.providerPayload("{\"a\":\"value\"}")
                .optionId(optionId)
                .build();
    }

    default PricingOption createSubscriptionPricingOption(String provider, BigDecimal price,
                                                          ProviderContentItem purchasingItem,
                                                          LogicalPeriod subscriptionPeriod) {
        return PricingOption.builder("title", PricingOptionType.SUBSCRIPTION, price, price, "RUB")
                .provider(provider)
                .providerPayload("{}")
                .purchasingItem(purchasingItem)
                .specialCommission(false)
                .subscriptionPeriod(subscriptionPeriod)
                .build();

    }
    default PricingOption createMBSubscriptionPricingOption(String provider, BigDecimal price,
                                                          ProviderContentItem purchasingItem,
                                                          LogicalPeriod subscriptionPeriod) {
        return PricingOption.builder("title", PricingOptionType.SUBSCRIPTION, price, price, "RUB")
                .provider(provider)
                .providerPayload("{}")
                .purchasingItem(purchasingItem)
                .specialCommission(false)
                .subscriptionPeriod(subscriptionPeriod)
                .processor(PaymentProcessor.MEDIABILLING)
                .build();

    }

    default PricingOption createSubscriptionPricingOptionWithIntroPeriod(String provider, BigDecimal price,
                                                                         BigDecimal firstPaymentPrice,
                                                                         ProviderContentItem purchasingItem,
                                                                         LogicalPeriod subscriptionPeriod,
                                                                         LogicalPeriod firstPaymentPeriod) {
        return PricingOption.builder("title", PricingOptionType.SUBSCRIPTION, firstPaymentPrice, price, "RUB")
                .provider(provider)
                .providerPayload("{}")
                .purchasingItem(purchasingItem)
                .specialCommission(false)
                .subscriptionPeriod(subscriptionPeriod)
                .discountType(PricingOption.DiscountType.FIRST_PAYMENT)
                .subscriptionTrialPeriod(firstPaymentPeriod)
                .build();

    }
}

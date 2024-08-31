package ru.yandex.quasar.billing.services;

import ru.yandex.quasar.billing.beans.PricingOption;
import ru.yandex.quasar.billing.beans.ProviderContentItem;

/**
 * Native providers purchases require additional logic before and after {@link BillingService} calls.
 * The service adds provider specific logic around more general {@link BillingService}'s methods.
 */
public interface ProviderPurchaseService {


    /**
     * Initialize purchase of the {@param providerContentItem} using {@param selectedOption}
     *
     * @return purchaseToken for further status check
     */
    String initPurchaseItem(String uid, String providerName, ProviderContentItem providerContentItem,
                            PricingOption selectedOption, String userIp, String paymentCardId, String userEmail);


}

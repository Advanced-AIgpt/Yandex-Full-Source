package ru.yandex.quasar.billing.services;

import java.time.Duration;
import java.time.Instant;
import java.util.Map;
import java.util.Optional;

import org.springframework.beans.factory.InitializingBean;
import org.springframework.stereotype.Service;

import ru.yandex.quasar.billing.beans.PaymentProcessor;
import ru.yandex.quasar.billing.beans.PricingOption;
import ru.yandex.quasar.billing.beans.PricingOptionType;
import ru.yandex.quasar.billing.beans.ProviderContentItem;
import ru.yandex.quasar.billing.dao.PurchaseInfo;
import ru.yandex.quasar.billing.exception.ExpiredException;
import ru.yandex.quasar.billing.exception.ProviderUnauthorizedException;
import ru.yandex.quasar.billing.providers.IContentProvider;
import ru.yandex.quasar.billing.providers.ProviderActiveSubscriptionInfo;
import ru.yandex.quasar.billing.providers.ProviderManager;
import ru.yandex.quasar.billing.providers.ProviderPricingOptions;
import ru.yandex.quasar.billing.providers.ProviderPurchaseException;
import ru.yandex.quasar.billing.providers.universal.UniversalProviderConfigurator;

@Service
public class ProviderPurchaseServiceImpl implements ProviderPurchaseService, InitializingBean {
    private final ProviderManager providerManager;
    private final AuthorizationService authorizationService;
    private final BillingService billingService;

    public ProviderPurchaseServiceImpl(ProviderManager providerManager,
                                       AuthorizationService authorizationService,

                                       BillingService billingService) {
        this.providerManager = providerManager;
        this.authorizationService = authorizationService;
        this.billingService = billingService;
    }

    @Override
    public void afterPropertiesSet() {
        providerManager.getAllContentProviders().stream()
                .map(IContentProvider::getProviderName)
                .forEach(name -> billingService.registerCallback(name, this::onPurchaseEvent));
    }

    @Override
    public String initPurchaseItem(String uid, String providerName, ProviderContentItem providerContentItem,
                                   PricingOption selectedOption, String userIp, String paymentCardId,
                                   String userEmail) {
        IContentProvider provider = providerManager.getContentProvider(providerName);

        try {
            String session = authorizationService.getProviderTokenByUid(uid, provider.getSocialAPIServiceName())
                    .orElseThrow(() -> new ProviderUnauthorizedException("Provider account not bound", providerName,
                            false));

            // Check is the contentItem we are going to purchase is still the same.
            // User might has started the purchase process not being logged-in and it also may lead to the offer
            // parameters change
            PricingOption actualSelectedOption = refreshPricingOption(provider, providerContentItem, selectedOption,
                    session)
                    .orElseThrow(() -> new ExpiredException("Pricing option has expired"));

            if (actualSelectedOption.getType() == PricingOptionType.SUBSCRIPTION &&
                    provider.getActiveSubscriptions(session).containsKey(providerContentItem)) {
                return billingService.createExceptionalPurchase(uid, providerName,
                        PurchaseInfo.Status.ALREADY_AVAILABLE, providerContentItem, selectedOption, null,
                        PaymentProcessor.TRUST);
            } else {
                return billingService.initPurchase(uid, providerName, actualSelectedOption, providerContentItem,
                        userIp, paymentCardId, userEmail, null);
            }

        } catch (ProviderUnauthorizedException e) {
            return billingService.createExceptionalPurchase(uid, providerName,
                    PurchaseInfo.Status.ERROR_NO_PROVIDER_ACC, providerContentItem, selectedOption, null,
                    PaymentProcessor.TRUST);
        } catch (ExpiredException e) {
            return billingService.createExceptionalPurchase(uid, providerName,
                    PurchaseInfo.Status.ERROR_PAYMENT_OPTION_OBSOLETE, providerContentItem, selectedOption, null,
                    PaymentProcessor.TRUST);
        }
    }

    private Optional<PricingOption> refreshPricingOption(IContentProvider contentProvider,
                                                         ProviderContentItem purchasingItem,
                                                         PricingOption selectedOption, String session) {
        ProviderPricingOptions pricingOptions = contentProvider.getPricingOptions(purchasingItem, session);

        return pricingOptions.getPricingOptions().stream()
                .filter(that -> that.equalsForBilling(selectedOption))
                .findFirst();
    }


    private void onPurchaseEvent(String provider, String uid, ProviderContentItem contentItem,
                                 PricingOption selectedOption, String orderId, String purchaseToken)
            throws ProviderPurchaseException {

        IContentProvider contentProvider = providerManager.getContentProvider(provider);

        // Check if session exists. see QUASAR-1724, QUASAR-1930
        String session = authorizationService.getProviderTokenByUid(uid, contentProvider.getSocialAPIServiceName())
                .orElseThrow(() -> new ProviderUnauthorizedException(contentProvider.getProviderName(), false));


        PricingOption actualPricingOption;

        if (UniversalProviderConfigurator.KINOPOISK_PROVIDER_NAME.equals(provider)) {
            // kinopoisk requires user ticket to get price/check availability. On TRUST callback we don't have one
            actualPricingOption = selectedOption;
        } else {
            // Check pricing and purchasing item hasn't changed
            // As we are finishing the purchase process we check purchasing item for being actual (offer hasn't
            // expired, price hasn't changed etc)
            // Purchasing item is the one we actually purchase, not necessary the one we would like to obtain.
            // For example we might purchase subscription (purchasingItem) to access a film (providerContentItem)
            actualPricingOption = refreshPricingOption(contentProvider, selectedOption.getPurchasingItem(),
                    selectedOption, session)
                    .orElseThrow(() ->
                            new ProviderPurchaseException(PurchaseInfo.Status.ERROR_PAYMENT_OPTION_OBSOLETE));

            if (actualPricingOption.getType() == PricingOptionType.SUBSCRIPTION) {
                // Для подписок проверяем срок действия на стороне провайдера - если осталось больше суток то не даём
                // покупать/продлевать
                Map<ProviderContentItem, ProviderActiveSubscriptionInfo> activeSubscriptions =
                        contentProvider.getActiveSubscriptions(session);

                var activeSubscriptionInfo = activeSubscriptions.get(actualPricingOption.getPurchasingItem());
                Instant activeTill = activeSubscriptionInfo != null ? activeSubscriptionInfo.getActiveTill() : null;
                if (activeTill != null && activeTill.isAfter(Instant.now().plus(Duration.ofDays(1)))) {
                    throw new ProviderPurchaseException(PurchaseInfo.Status.ALREADY_AVAILABLE);
                }
            }
        }

        // transactional purchase is not rechecked as it needs user's IP and agent but we don't have it
        // as currently TRUST callback is processed. Check should be performed either before hold or on provider's side

        // проводим покупку на стороне провайдера
        contentProvider.processPurchase(actualPricingOption.getPurchasingItem(), actualPricingOption, purchaseToken,
                session);

    }


}

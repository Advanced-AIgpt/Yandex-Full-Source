package ru.yandex.quasar.billing.services.processing;

import java.util.EnumMap;
import java.util.Map;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.transaction.PlatformTransactionManager;

import ru.yandex.quasar.billing.beans.PaymentProcessor;
import ru.yandex.quasar.billing.config.BillingConfig;
import ru.yandex.quasar.billing.dao.GenericProductDao;
import ru.yandex.quasar.billing.dao.SubscriptionProductsDAO;
import ru.yandex.quasar.billing.dao.UserPurchasesDAO;
import ru.yandex.quasar.billing.services.AuthorizationContext;
import ru.yandex.quasar.billing.services.mediabilling.MediaBillingClient;
import ru.yandex.quasar.billing.services.mediabilling.MediaBillingService;
import ru.yandex.quasar.billing.services.processing.trust.PaymentMethodsClient;
import ru.yandex.quasar.billing.services.processing.trust.TrustBillingClient;
import ru.yandex.quasar.billing.services.processing.trust.TrustBillingService;
import ru.yandex.quasar.billing.services.processing.yapay.YaPayBillingService;
import ru.yandex.quasar.billing.services.processing.yapay.YandexPayClient;

@Configuration
public class ProcessingServicesConfig {

    private final BillingConfig config;
    private final SubscriptionProductsDAO subscriptionProductsDAO;
    private final GenericProductDao genericProductDao;
    private final PlatformTransactionManager transactionManager;


    @Autowired
    public ProcessingServicesConfig(
            BillingConfig billingConfig,
            SubscriptionProductsDAO subscriptionProductsDAO,
            GenericProductDao genericProductDao,
            PlatformTransactionManager transactionManager) {
        this.config = billingConfig;
        this.subscriptionProductsDAO = subscriptionProductsDAO;
        this.genericProductDao = genericProductDao;
        this.transactionManager = transactionManager;
    }


    @Bean("trustBillingService")
    TrustBillingService trustBillingService(@Qualifier("trustBillingClient") TrustBillingClient trustClient) {
        return new TrustBillingService(
                PaymentProcessor.TRUST,
                config,
                trustClient,
                genericProductDao,
                transactionManager);
    }

    @Bean("yandexPayBillingService")
    YaPayBillingService yandexPayBillingService(YandexPayClient client, TrustBillingService trustBillingService,
                                                AuthorizationContext authorizationContext) {
        return new YaPayBillingService(client, trustBillingService, config, authorizationContext);
    }

    @Bean("freeProcessingBillingService")
    FreeProcessingBillingService freeProcessingBillingService(
            TrustBillingService trustBillingService,
            UserPurchasesDAO userPurchasesDAO
    ) {
        return new FreeProcessingBillingService(trustBillingService, userPurchasesDAO);
    }

    @Bean("mediaBillingService")
    MediaBillingService mediaBillingService(
            MediaBillingClient mediaBillingClient,
            @Qualifier("mediaBillingTrustClient") PaymentMethodsClient cardsListClient) {
        return new MediaBillingService(mediaBillingClient, cardsListClient);
    }

    @Bean
    ProcessingServiceManager processingServiceManager(
            @Qualifier("trustBillingService") ProcessingService trustBillingService,
            YaPayBillingService yandexPayBillingService,
            MediaBillingService mediaBillingService,
            FreeProcessingBillingService freeProcessingBillingService
    ) {
        Map<PaymentProcessor, ProcessingService> processingMap = new EnumMap<>(PaymentProcessor.class);
        processingMap.put(PaymentProcessor.TRUST, trustBillingService);
        processingMap.put(PaymentProcessor.YANDEX_PAY, yandexPayBillingService);
        processingMap.put(PaymentProcessor.MEDIABILLING, mediaBillingService);
        processingMap.put(PaymentProcessor.FREE, freeProcessingBillingService);

        return new ProcessingServiceManager(processingMap);
    }
}

package ru.yandex.quasar.billing.services.processing.trust;

import org.springframework.boot.web.client.RestTemplateBuilder;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import ru.yandex.passport.tvmauth.TvmClient;
import ru.yandex.quasar.billing.config.BillingConfig;
import ru.yandex.quasar.billing.config.MusicApiConfig;
import ru.yandex.quasar.billing.config.TrustBillingConfig;

@Configuration
public class TrustClientConfig {
    private final BillingConfig config;
    private final RestTemplateBuilder restTemplateBuilder;
    private final TvmClient tvmClient;

    public TrustClientConfig(
            BillingConfig config,
            RestTemplateBuilder restTemplateBuilder,
            TvmClient tvmClient
    ) {
        this.config = config;
        this.restTemplateBuilder = restTemplateBuilder;
        this.tvmClient = tvmClient;
    }

    @Bean("trustBillingClient")
    TrustBillingClient trustBillingClient() {
        TrustBillingConfig trustBillingConfig = config.getTrustBillingConfig();
        String tvmClientId = trustBillingConfig.getTvmClientId();
        return new TrustBillingClientImpl(
                trustBillingConfig.getApiBaseUrl(),
                restTemplateBuilder,
                tvmClient,
                trustBillingConfig.getConnectTimeout(),
                trustBillingConfig.getTimeout(),
                trustBillingConfig.getServiceToken(),
                tvmClientId,
                true
        );
    }

    @Bean("mediaBillingTrustClient")
    PaymentMethodsClient mediaBillingTrustClient() {
        MusicApiConfig musicApiConfig = config.getMusicApiConfig();
        TrustBillingConfig trustBillingConfig = config.getTrustBillingConfig();
        boolean useTvm = !musicApiConfig.isUseTrustToken();

        return new TrustBillingClientImpl(
                musicApiConfig.getTrustBaseApi(),
                restTemplateBuilder,
                tvmClient,
                musicApiConfig.getTrustConnectTimeout(),
                musicApiConfig.getTrustTimeout(),
                trustBillingConfig.getServiceToken(),
                trustBillingConfig.getTvmClientId(),
                useTvm
        );
    }

}

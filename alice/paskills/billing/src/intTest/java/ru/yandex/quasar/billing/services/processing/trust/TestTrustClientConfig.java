package ru.yandex.quasar.billing.services.processing.trust;

import org.springframework.boot.test.context.TestConfiguration;
import org.springframework.boot.web.client.RestTemplateBuilder;
import org.springframework.context.annotation.Bean;

import ru.yandex.passport.tvmauth.TvmClient;
import ru.yandex.quasar.billing.config.BillingConfig;

@TestConfiguration("trustClientConfig")
public class TestTrustClientConfig extends TrustClientConfig {

    private final TestTrustClient trustBillingClient;

    public TestTrustClientConfig(
            BillingConfig config,
            RestTemplateBuilder restTemplateBuilder,
            TvmClient tvmClient,
            TestTrustClient trustBillingClient) {
        super(config, restTemplateBuilder, tvmClient);
        this.trustBillingClient = trustBillingClient;
    }

    @Bean("trustBillingClient")
    @Override
    TrustBillingClient trustBillingClient() {
        return trustBillingClient;
    }

    @Bean("mediaBillingTrustClient")
    @Override
    PaymentMethodsClient mediaBillingTrustClient() {
        return trustBillingClient;
    }

}
